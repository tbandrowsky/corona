/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved

MIT License

About this File
Wraps DirectX devices.

Notes

For Future Consideration
*/

#pragma once

namespace corona
{

	class direct3dDevice : public std::enable_shared_from_this<direct3dDevice>
	{
		ID3D11Device* d3d11Device;
		D3D_FEATURE_LEVEL	feature_level;

	public:

		direct3dDevice()
		{
			d3d11Device = nullptr;
			feature_level = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
		}

		~direct3dDevice()
		{
			if (d3d11Device)
				d3d11Device->Release();
			d3d11Device = nullptr;
		}

		bool setDevice(IDXGIAdapter1* _adapter)
		{
			// Release any existing device owned by this object.
			if (d3d11Device)
			{
				d3d11Device->Release();
				d3d11Device = nullptr;
			}

			D3D_FEATURE_LEVEL feature_levels[] = {
				D3D_FEATURE_LEVEL_12_1,
				D3D_FEATURE_LEVEL_12_0,
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0
			};

			// If an adapter is supplied, the driver type MUST be D3D_DRIVER_TYPE_UNKNOWN.
			// If adapter is NULL, use the hardware driver type.
			D3D_DRIVER_TYPE driverType = (_adapter != nullptr) ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;

			ID3D11Device* createdDevice = nullptr;
			ID3D11DeviceContext* createdContext = nullptr;

			HRESULT hr = D3D11CreateDevice(
				_adapter,                               // pAdapter
				driverType,                             // DriverType
				NULL,                                   // Software (only valid for D3D_DRIVER_TYPE_SOFTWARE)
				D3D11_CREATE_DEVICE_BGRA_SUPPORT,       // Flags
				feature_levels,                         // pFeatureLevels
				sizeof(feature_levels) / sizeof(feature_levels[0]), // FeatureLevels
				D3D11_SDK_VERSION,                      // SDKVersion
				&createdDevice,                         // ppDevice
				&feature_level,                         // pFeatureLevel
				NULL                          // ppImmediateContext (optional)
			);

			if (SUCCEEDED(hr) && createdDevice != nullptr)
			{
				// Store the created device in the class member (avoid shadowing).
				d3d11Device = createdDevice;
				return true;
			}

			// Ensure partial objects are released on failure.
			if (createdDevice)
			{
				createdDevice->Release();
				createdDevice = nullptr;
			}

			return false;
		}

		ID3D11Device* getD3DDevice() { return d3d11Device; }
		D3D_FEATURE_LEVEL getFeatureLevel() { return feature_level; }
	};

	class direct2dDevice : public std::enable_shared_from_this<direct2dDevice>
	{
		IDXGIDevice* dxDevice;
		ID2D1Device* d2dDevice;

		ID2D1Factory1* d2DFactory;
		IWICImagingFactory* wicFactory;
		IDWriteFactory* dWriteFactory;

	public:

		direct2dDevice()
		{
			dxDevice = nullptr;
			d2dDevice = nullptr;

			d2DFactory = nullptr;
			wicFactory = nullptr;
			dWriteFactory = nullptr;

			D2D1_FACTORY_OPTIONS options = {};
//			options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
			HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, &d2DFactory);
			throwOnFail(hr, "Could not create D2D1 factory");

			hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
			throwOnFail(hr, "Could not create WIC Imaging factory");

			hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(dWriteFactory), reinterpret_cast<IUnknown**>(&dWriteFactory));
			throwOnFail(hr, "Could not create direct write factory");
		}

		~direct2dDevice()
		{
			if (wicFactory)
				wicFactory->Release();
			if (dWriteFactory)
				dWriteFactory->Release();

			if (dxDevice)
				dxDevice->Release();
			if (d2dDevice)
				d2dDevice->Release();
			if (d2DFactory)
				d2DFactory->Release();

			dxDevice = nullptr;
			d2dDevice = nullptr;

			d2DFactory = nullptr;
			wicFactory = nullptr;
			dWriteFactory = nullptr;

		}

		bool setDevice(ID3D11Device* _d3dDevice)
		{
			// Query IDXGIDevice from the D3D11 device safely using IID_PPV_ARGS.
			HRESULT hr = _d3dDevice->QueryInterface(IID_PPV_ARGS(&this->dxDevice));
			if (FAILED(hr) || this->dxDevice == nullptr)
				return false;

			hr = d2DFactory->CreateDevice(dxDevice, &d2dDevice);

			return SUCCEEDED(hr);
		}

		ID2D1Factory1* getD2DFactory() { return d2DFactory; }
		IWICImagingFactory* getWicFactory() { return wicFactory; }
		IDWriteFactory* getDWriteFactory() { return dWriteFactory; }
		ID2D1Device* getD2DDevice() { return d2dDevice; }

	};

}

