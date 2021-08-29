#pragma once

struct GLFWwindow;

namespace pt {

void CreateMainWindow( int width, int height );
void CloseWindow();
bool ShouldCloseWindow();
void DestroyMainWindow();
void PollEvents();
void SwapBuffers();
void GetDisplaySize( int* width, int* height );

GLFWwindow* GetInternalWindow();

}  // namespace pt
