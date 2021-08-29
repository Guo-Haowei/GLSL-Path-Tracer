#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "../third_party/imgui/imgui.h"
#include "application.h"
#include "camera.h"
#include "com_misc.h"
#include "constant_cache.h"
#include "glutil.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "viewer.h"

using namespace pt;

static Viewer g_viewer;

int main( int argc, const char** argv )
{
    Com_RegisterDvars();
    Com_ProcessCmdLine( argc - 1, argv + 1 );

    try
    {
        g_viewer.Initialize();
        while ( !ShouldCloseWindow() )
        {
            PollEvents();
            g_viewer.Update();
            SwapBuffers();
        }
        g_viewer.Finalize();
    }
    catch ( std::runtime_error& err )
    {
        printf( "Exception: %s\n", err.what() );
        return 1;
    }

    return 0;
}
