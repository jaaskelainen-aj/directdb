/* This file is part of 'Direct Database' C++ library (directdb)
 * https://github.com/jaaskelainen-aj/directdb
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#include <iostream>
#include <cpp4scripts.hpp>

using namespace std;
using namespace c4s;

program_arguments args;

// -------------------------------------------------------------------------------------------------
#if defined(__linux) || defined(__APPLE__)
BUILD_STATUS
Build()
{
    builder_gcc make("directdb", &cout);
    make.set(BUILD::LIB);
    make.add(args.is_set("-deb") ? BUILD::DEB : BUILD::REL);
    if (args.is_set("-V"))
        make.add(BUILD::VERBOSE);
    if (args.is_set("-ccdb"))
        make.add(BUILD::EXPORT);
    //  TODO: have the ccdb path setting to set flags as well.
    make.add_comp("-Wno-ctor-dtor-privacy -Wnon-virtual-dtor -I/usr/local/include/cpp4scripts "
                  "-I/usr/include/postgresql "
                  "-I/usr/local/include/sqlite3");
    make.add_comp("-fno-rtti");
    if (args.is_set("-deb"))
        make.add_comp("-DC4S_LOG_LEVEL=1");
    else
        make.add_comp("-DC4S_LOG_LEVEL=3");
    return make.build();
}
// -------------------------------------------------------------------------------------------------
#else
BUILD_STATUS
Build()
{
    builder_vc make("directdb", &cout);
    BUILD flags(BUILD::LIB);
    make.set(BUILD::LIB);
    make.add(args.is_set("-deb") ? BUILD::DEB : BUILD::REL);
    if (args.is_set("-V"))
        make.add(BUILD::VERBOSE);
    // make.add_comp("/DUSE_SSL /I$(BINC)\\cpp4scripts /I$(BINC)\\libpq"); // /I$(LIBPQ)\\include
    if (args.is_set("-deb")) {
        make.add_comp("/DC4S_LOG_LEVEL=2");
        if (!wxmode)
            make.add_comp("/D_DEBUG");
    } else
        make.add_comp("/DC4S_LOG_LEVEL=4");
    return make.build();
}
#endif
// -------------------------------------------------------------------------------------------------
int
Clean()
{
    try {
        path("./debug/").rmdir(true);
        path("./release/").rmdir(true);
    } catch (const path_exception& pe) {
        cerr << pe.what() << endl;
        return 1;
    }
    cout << "Build directories cleaned!\n";
    return 0;
}
// -------------------------------------------------------------------------------------------------
int
Install()
{
    path lib_d("./debug/libdirectdb.a");
    path lib_r("./release/libdirectdb.a");
    int count = 0;
    try {
        string root = append_slash(args.get_value("-install"));
        if (lib_d.exists()) {
            cout << "Copying " << lib_d.get_path() << '\n';
            lib_d.cp(path(root + "lib-d/"), PCF_FORCE);
            count++;
        }
        if (lib_r.exists()) {
            cout << "Copying " << lib_r.get_path() << '\n';
            lib_r.cp(path(root + "lib/"), PCF_FORCE);
            count++;
        }
        if (count == 0)
            cout << "Warning: No libraries copied. Did you build first?\n";
	path incdir(root+"include/directdb/");
	if(!incdir.dirname_exists())
	  incdir.mkdir();
        path_list ipl(path("./"), "\\.hpp$");
        ipl.copy_to(incdir, PCF_FORCE);
    } catch (const c4s_exception& ce) {
        cout << "Install failed: " << ce.what() << '\n';
        return 1;
    }
    return 0;
}
// -------------------------------------------------------------------------------------------------
int
main(int argc, char** argv)
{
    BUILD_STATUS rv = BUILD_STATUS::OK;
    string mode, binding;

    // Parameters
    args += argument("-deb", false, "Sets the debug mode");
    args += argument("-rel", false, "Sets the release mode");
    args += argument("-ccdb", true, "Creates compiler_commands.json with given directory.");
    args += argument("-V", false, "Enable verbose build mode");
    args += argument("-install", true, "Install library to given root.");
    args += argument("-clean", false, "Clean up build files.");

    cout << "Direct Database Library build v 2.0\n";
    try {
        args.initialize(argc, argv, 1);
    } catch (const c4s_exception& ce) {
        cerr << "Error: " << ce.what() << endl;
        args.usage();
        return 1;
    }

    if (args.is_set("-clean"))
        return Clean();

    if (args.is_set("-install"))
        return Install();

    try {
        rv = Build();
    } catch (const c4s_exception& ce) {
        cout << "Build failed: " << ce.what() << '\n';
    }
    cout << "Build finished.\n";
    return (int) rv;
}
