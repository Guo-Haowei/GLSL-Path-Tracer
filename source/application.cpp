#include "application.h"

#include <GLFW/glfw3.h>

#include <stdexcept>

extern GLFWwindow* g_Window;

namespace pt {

GLFWwindow* GetInternalWindow()
{
    return g_Window;
}

void CreateMainWindow( int width, int height )
{
    glfwSetErrorCallback( []( int, const char* error ) {
        throw std::runtime_error( error );
    } );
    glfwInit();
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

    g_Window = glfwCreateWindow( width, height, "GLSL path tracer", 0, 0 );
    glfwMakeContextCurrent( g_Window );
    glfwSwapInterval( 1 );
}

void DestroyMainWindow()
{
    glfwDestroyWindow( g_Window );
    glfwTerminate();
}

void CloseWindow()
{
    glfwSetWindowShouldClose( g_Window, GLFW_TRUE );
}

bool ShouldCloseWindow()
{
    return glfwWindowShouldClose( g_Window );
}

void PollEvents()
{
    glfwPollEvents();
}

void SwapBuffers()
{
    glfwSwapBuffers( g_Window );
}

void GetDisplaySize( int* width, int* height )
{
    glfwGetFramebufferSize( g_Window, width, height );
}

}  // namespace pt
