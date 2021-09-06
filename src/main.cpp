#include <iostream>

#define BIT7Z_AUTO_FORMAT

#include <filesystem>
#include <fstream>

#include <bitarchiveinfo.hpp>
#include <bitextractor.hpp>
#include <bitexception.hpp>

namespace fs = std::filesystem;

/** 
* Return the filename without any extension(s)
* i.e. "foo.tar.gz" => "foo"
*/
std::filesystem::path filename_without_extensions (std::filesystem::path filename)
{
    typedef typename std::filesystem::path::string_type string_type;

    if (filename.empty())
        return {};

    const string_type str = filename;

    size_t dotpos = str.find_first_of('.');
    if (dotpos == string_type::npos)
    {
        return filename;
    }

    return str.substr (0, dotpos);
}

/** 
* Return the extension(s), including the dot, of a provided filename,
* i.e. "foo.tar.gz" => "tar.gz"
*/
std::filesystem::path filename_extensions (std::filesystem::path filename)
{
    typedef typename std::filesystem::path::string_type string_type;

    if (filename.empty())
        return {};

    const string_type str = filename;

    size_t dotpos = str.find_first_of('.');
    if (dotpos == string_type::npos)
    {
        return {};
    }

    return fs::path { str.substr (dotpos) };
}

/**
* Return target_path, or if such file or directory already exists in the filesystem,
* increment the name with " (i)" pattern until non-existing path is found.
*/
std::wstring find_nonexisting_sibling_path (const fs::path& target_path)
{
    if (! fs::exists(target_path))
    {
        return target_path;
    }

    fs::path dir = target_path.parent_path();
    fs::path filename = target_path.filename();

    // if target_path is a directory
    if (filename.empty())
    {
        filename = *--dir.end(); // traverse down one path level for name
        dir = dir.parent_path();
    }

    // separate extension(s) from filename

    fs::path extensions = filename_extensions (filename);
    filename = filename_without_extensions (filename);

    std::cout << dir << std::endl;
    std::cout << filename << std::endl;
    std::cout << extensions << std::endl;

    int increment = 1;
    fs::path candidate;

    for (;;)
    {
        wchar_t wbuf[16];

        if (swprintf_s(wbuf, 16U, L" (%u)", increment) < 0)
        {
            throw bit7z::BitException("Error occurred when trying to find extraction destination");
        }

        candidate = dir / filename;
        candidate += wbuf;
        candidate += extensions;

        if (! fs::exists(candidate))
            break;

        increment++;
    } 

    return candidate;
}

int wmain(int argc, wchar_t *argv[])
{
    using namespace bit7z;

    if (argc != 2)
    {
        return 1;
    }

    fs::path archive_path = fs::absolute(fs::path { argv[1] });
    std::cout << archive_path << std::endl;

    {
        // TODO extension protection
        std::wstring extensions = filename_extensions(archive_path.filename());
    }

    //std::cout << in_path.parent_path() << std::endl;
    //std::cout << in_path.parent_path().stem() << std::endl;

    try
    {
        Bit7zLibrary lib { L"7z.dll" };
        BitArchiveInfo archive { lib, archive_path, BitFormat::Auto };

        //printing archive metadata
        std::wcout << L"Archive properties" << std::endl;
        std::wcout << L" Items count: "   << archive.itemsCount() << std::endl;
        std::wcout << L" Folders count: " << archive.foldersCount() << std::endl;
        std::wcout << L" Files count: "   << archive.filesCount() << std::endl;
        std::wcout << L" Size: "          << archive.size() << std::endl;
        std::wcout << L" Packed size: "   << archive.packSize() << std::endl;
        std::wcout << std::endl;


        size_t num_items = archive.itemsCount();

        if (num_items == 0)
        {
            // empty archive, do nothing
            return 0;
        }

        auto archive_items = archive.items();
        BitExtractor extractor { lib };

        // if input archive has only one item
            // get destination path of the file we want to extract
            // find unique file name (incrementing)
            // extract using the stream extractor

        if (num_items == 1)
        {
            fs::path dst_path = fs::absolute (fs::path { archive_items[0].name() });
            dst_path = find_nonexisting_sibling_path (dst_path);
            std::cout << dst_path << std::endl;

            std::ofstream ofs (dst_path, std::ios::binary);

            extractor.extract(archive_path, ofs);
        }

#if 0
        for (auto& item : archive_items) {
            std::wcout << std::endl;
            std::wcout << L" Item index: "   << item.index() << std::endl;
            std::wcout << L"  Name: "        << item.name() << std::endl;
            std::wcout << L"  Extension: "   << item.extension() << std::endl;
            std::wcout << L"  Path: "        << item.path() << std::endl;
            std::wcout << L"  IsDir: "       << item.isDir() << std::endl;
            std::wcout << L"  Size: "        << item.size() << std::endl;
            std::wcout << L"  Packed size: " << item.packSize() << std::endl;
        }
#endif

        // find if more than one file exists at the root of the archive

        bool archive_items_have_common_root_dir = true;
        fs::path archive_root_dir;
        for (auto& item : archive_items)
        {
            fs::path rel_path { item.path() };

            if (archive_root_dir.empty())
            {
                archive_root_dir= *rel_path.begin();
            }
            else if ((archive_root_dir == *rel_path.begin()) == false)
            {
                archive_items_have_common_root_dir = false;
                break;
            }
        }

        // if input archive has more than 1 item at the root
            // find nonexisting folder name derived from archive name (sans extensions)
            // create directory 
            // extract archive to the newly created directory as is

        if (! archive_items_have_common_root_dir)
        {
            fs::path parent_dir = archive_path.parent_path();
            fs::path output_dir = parent_dir / filename_without_extensions (archive_path);
            output_dir = find_nonexisting_sibling_path (output_dir);

            if (fs::create_directory (output_dir))
            {
                extractor.extract(archive_path, output_dir);
            }
            else
            {
                throw bit7z::BitException("Unable to create directory for extracted archive.");
            }
        }

        // if input archive items have a common root directory
            // if expected root dir doesn't exist
                // extract normally to parent dir
            // else
                // find nonexisting destination dir, create
                // extract inside the new directory
                // move extracted to parent_dir with temp name
                // rename temp as destination dir

        else 
        {
            fs::path parent_dir = archive_path.parent_path();
            fs::path destination_dir = parent_dir / archive_root_dir / fs::path {};

            if (! fs::exists(destination_dir))
            {
                extractor.extract(archive_path, parent_dir);
            }
            else
            {
                destination_dir = find_nonexisting_sibling_path (destination_dir);

                if (fs::create_directory (destination_dir))
                {
                    // directory that should be moved from being inside the destination_dir to replace the destination_dir
                    fs::path output_root_dir = destination_dir / archive_root_dir;

                    // fs::path doesn't allow moving directory to be in the same ancestry, we need to move extraction results
                    // out of the destination_dir first to then overwrite destination_dir
                    fs::path temp_root_dir = fs::path { destination_dir }.concat("_extracted") / fs::path {};

                    extractor.extract(archive_path, destination_dir);
                    fs::rename(output_root_dir, temp_root_dir);
                    fs::remove(destination_dir);
                    fs::rename(temp_root_dir, destination_dir);
                }
                else
                {
                    throw bit7z::BitException("Unable to create directory for extracted archive.");
                }
            }
        }
    }
    catch (const BitException& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    catch (const fs::filesystem_error& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    return 0;
}
