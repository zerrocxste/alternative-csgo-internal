#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <corecrt_io.h>
#include <iostream>
#include <fcntl.h>

#pragma comment (lib, "urlmon.lib")

#include <d3d9.h>
#pragma comment (lib, "d3d9.lib")

#include "Include/d3dx9.h"
#pragma comment (lib, "Lib/x86/d3dx9.lib")

#include "minhook.h"
#pragma comment (lib, "minhook.lib")

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"