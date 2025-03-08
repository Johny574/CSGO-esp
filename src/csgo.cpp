#include <d3d11.h>
#include <iostream>
#include <windows.h>
#include "vector.h"
#include "proc.h"
#include <sstream>
#include <wchar.h>

#include <dwmapi.h>
#include <d3d11.h>
#include <windowsx.h>
#include "../external/imGUI/imgui.h"
#include "../external/imGUI/imgui_impl_dx11.h"
#include "../external/imGUI/imgui_impl_win32.h"
#include "render.h"


float screenWidth = GetSystemMetrics(SM_CXSCREEN);
float screenHeight = GetSystemMetrics(SM_CYSCREEN);


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK windowprocedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
	{
		return 0L;
	}

	if (message == WM_DESTROY)
	{
		return 0L;
	}

	switch (message)
	{
	case WM_NCHITTEST:
	{
		const long borderWidth = GetSystemMetrics(SM_CXSIZEFRAME);
		const long titlebarHeight = GetSystemMetrics(SM_CXSIZEFRAME);
		POINT cursorPos = { GET_X_LPARAM(wParam), GET_Y_LPARAM(lParam) };
		RECT windowRect;
		GetWindowRect(hwnd, &windowRect);

		if (cursorPos.y >= windowRect.top && cursorPos.y < windowRect.top + titlebarHeight)
		{
			return HTCAPTION;
		}
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE instance, PSTR lpCmdLine, int cmd_show)
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = windowprocedure;
	wc.hInstance = instance;
	wc.lpszClassName = L"test";

	RegisterClassExW(&wc);
	const HWND overlay = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		wc.lpszClassName,
		L"test",
		WS_POPUP,
		0,
		0,
		screenWidth,
		screenHeight,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), BYTE(255), LWA_COLORKEY);

	{
		RECT clientArea{};
		GetClientRect(overlay, &clientArea);

		RECT windowArea{};
		GetWindowRect(overlay, &windowArea);

		POINT diff;
		ClientToScreen(overlay, &diff);

		const MARGINS margins
		{
			windowArea.left + (diff.x - windowArea.left),
			windowArea.top + (diff.y - windowArea.top),
			clientArea.right,
			clientArea.bottom
		};

		DwmExtendFrameIntoClientArea(overlay, &margins);
	}
	std::cout << "set window";

	// swap chain
	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 10;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.SampleDesc.Count = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;
	sd.OutputWindow = overlay;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	std::cout << "created swap chain";

	constexpr D3D_FEATURE_LEVEL levels[2]
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};

	ID3D11Device* device{ nullptr };
	ID3D11DeviceContext* deviceContext{ nullptr };
	IDXGISwapChain* swapChain{ nullptr };
	ID3D11RenderTargetView* renderTarget{ nullptr };
	D3D_FEATURE_LEVEL level{};

	D3D11CreateDeviceAndSwapChain
	(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		levels,
		2U,
		D3D11_SDK_VERSION,
		&sd,
		&swapChain,
		&device,
		&level,
		&deviceContext
	);
	std::cout << "created device";

	ID3D11Texture2D* back_buffer{ nullptr };
	swapChain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));
	if (back_buffer)
	{
		device->CreateRenderTargetView(back_buffer, nullptr, &renderTarget);
		back_buffer->Release();
		deviceContext->OMSetRenderTargets(1, &renderTarget, nullptr);
	}
	else
	{
		std::cout << "noback buffer";
	}
	ShowWindow(overlay, cmd_show);
	UpdateWindow(overlay);

	ImGui::CreateContext();
	ImGui::StyleColorsClassic();

	ImGui_ImplWin32_Init(overlay);
	ImGui_ImplDX11_Init(device, deviceContext);
	
	bool running = true;

	ProcH proc(L"cs2.exe");
	uintptr_t clientbase = proc.GetModuleBaseAddress(L"client.dll");

	uintptr_t entityList;
	ReadProcessMemory(proc.Proc, (BYTE*)clientbase + 0x1A36A00, &entityList, sizeof(entityList), nullptr);


	uintptr_t listEntry;
	ReadProcessMemory(proc.Proc, (BYTE*)entityList + 0x10, &listEntry, sizeof(listEntry), nullptr);

	uintptr_t localPlayer;
	ReadProcessMemory(proc.Proc, (BYTE*)clientbase + 0x188AF20, &localPlayer, sizeof(localPlayer), nullptr);

	while (running)
	{
		ViewMatrix viewMatrix;
		ReadProcessMemory(proc.Proc, (BYTE*)clientbase + 0x1AA27F0, &viewMatrix, sizeof(viewMatrix), nullptr);
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				running = false;
			}
		}

		if (!running)
		{
			break;
		}
		
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		
		for (size_t i = 0; i < 64; i++)
		{
			uintptr_t controller = 0;
			ReadProcessMemory(proc.Proc, (BYTE*)listEntry + (i * 0x78), &controller, sizeof(controller), nullptr);

			if (controller == 0)
			{
				continue;
			}

			int handle = 0;
			ReadProcessMemory(proc.Proc, (BYTE*)controller + 0x80c, &handle, sizeof(handle), nullptr);

			if (handle == 0)
			{
				continue;
			}

			uintptr_t listEntry2 = 0;
			ReadProcessMemory(proc.Proc, (BYTE*)entityList + 0x8 * ((handle & 0x7FFF) >> 9) + 0x10, &listEntry2, sizeof(listEntry2), nullptr);

			if (listEntry2 == 0)
			{
				continue;
			}

			uintptr_t pawn = 0;
			ReadProcessMemory(proc.Proc, (BYTE*)listEntry2 + 120 * (handle & 0x1FF), &pawn, sizeof(pawn), nullptr);

			if (pawn == 0)
			{
				continue;
			}

			if (pawn == localPlayer)
			{
				continue;
			}

			int hp = 0;
			ReadProcessMemory(proc.Proc, (BYTE*)pawn + 0x344, &hp, sizeof(hp), nullptr);

			if (hp == 0)
			{
				continue;
			}

			Vector3 origin;
			ReadProcessMemory(proc.Proc, (BYTE*)pawn + 0x1324, &origin, sizeof(origin), nullptr);

			Vector3 head = { origin.x, origin.y, origin.z + 75 };

			Vector3 screenpos = origin.WorldToScreen(viewMatrix);
			Vector3 screenhead = head.WorldToScreen(viewMatrix);

			float height = screenpos.y - screenhead.y;
			float width = height / 2.4f;

			Render::DrawRect(
				screenhead.x - width / 2,
				screenhead.y,
				width,
				height,
				{ 255,0,0 },
				1.5f
			);
		
			std::ostringstream oss;
			oss << "hp: " << hp;
			//MessageBoxA(nullptr, oss.str().c_str(), "uintptr_t Value", MB_OK | MB_ICONINFORMATION);
		}

		/*Render::DrawRect(
			500,
			500,
			100,
			100,
			{ 255,0,0 },
			1.5f
		);*/

		ImGui::Render();
		float color[4]{ 0, 0, 0, 0 };
		
		deviceContext->OMSetRenderTargets(1, &renderTarget, nullptr);
		deviceContext->ClearRenderTargetView(renderTarget, color);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		swapChain->Present(0U, 0U);
	}
}