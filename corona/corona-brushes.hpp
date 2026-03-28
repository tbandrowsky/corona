/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved

MIT License

About this File
Brushes in a corona device context.  Bitmap, solid and gradient brushes
are supported.

Notes

For Future Consideration
*/



#pragma once

namespace corona
{

	class brush
	{
	public:
		virtual ID2D1Brush* getBrush() = 0;
	};


	class bitmapBrush : public deviceDependentAsset<ID2D1BitmapBrush*>, brush {
	public:

		std::weak_ptr<bitmap> bm;

		bitmapBrush() : deviceDependentAsset()
		{
			;
		}

		virtual ~bitmapBrush()
		{

		}

		bool create(direct2dContextBase* ptarget)
		{
			HRESULT hr = HRESULT_FROM_WIN32(ERROR_BAD_ENVIRONMENT);
			if (ptarget) {

				if (!ptarget or !ptarget->getDeviceContext())
					return false;

				if (asset) {
					asset->Release();
					asset = nullptr;
				}

				if (auto pbm = bm.lock()) {
					hr = ptarget->getDeviceContext()->CreateBitmapBrush(pbm->getFirst(), &asset);
					if (!SUCCEEDED(hr)) {
						system_monitoring_interface::active_mon->log_warning("Could not create bitnap brush");
					}
				}

			}

			return SUCCEEDED(hr);
		}

		ID2D1Brush* getBrush()
		{
			return asset;
		}

		virtual std::string get_string() override
		{
			std::string value;
            if (bm.expired()) {
				value = "empty bitmap brush";
			}
			else if (auto pbm = bm.lock()) {
                value = "bitmap:"+ pbm->get_string();
			}
			return value;
		}
	};

	class solidColorBrush : public deviceDependentAsset<ID2D1SolidColorBrush*>, brush {
	public:

		D2D1_COLOR_F color;

		solidColorBrush() : deviceDependentAsset()
		{
			;
		}

		virtual ~solidColorBrush()
		{

		}

		bool create(direct2dContextBase* ptarget)
		{
			HRESULT hr = -1;

			if (ptarget)
			{
				if (!ptarget->getDeviceContext())
					return false;

				if (asset) {
					asset->Release();
					asset = nullptr;
				}

				hr = ptarget->getDeviceContext()->CreateSolidColorBrush(color, &asset);
			}

			if (!SUCCEEDED(hr)) {
				system_monitoring_interface::active_mon->log_warning("Could not create solid brush ");
			}

			return SUCCEEDED(hr);
		}

		virtual ID2D1Brush* getBrush()
		{
			return asset;
		}

		virtual std::string get_string() override
		{
			std::string value = std::format("solid:{0},{1},{2},{3}", color.r, color.g, color.b, color.a);
			return value;
		}
	};

	class linearGradientBrush : public deviceDependentAsset<ID2D1LinearGradientBrush*>, brush {
	public:
		std::vector<D2D1_GRADIENT_STOP> stops;
		D2D1_POINT_2F					start;
		D2D1_POINT_2F					stop;

		bool create(direct2dContextBase* ptarget)
		{
			ID2D1GradientStopCollection* pGradientStops = NULL;

			HRESULT hr = HRESULT_FROM_WIN32(ERROR_BAD_ENVIRONMENT);

			if (ptarget)
			{
				if (!ptarget->getDeviceContext())
					return false;

				if (stops.size() > 0) {
					hr = ptarget->getDeviceContext()->CreateGradientStopCollection(&stops[0], stops.size(), &pGradientStops);

					if (SUCCEEDED(hr))
					{
						if (asset) {
							asset->Release();
							asset = nullptr;
						}
						hr = ptarget->getDeviceContext()->CreateLinearGradientBrush(
							D2D1::LinearGradientBrushProperties(start, stop),
							D2D1::BrushProperties(),
							pGradientStops,
							&asset
						);
						pGradientStops->Release();
						if (!SUCCEEDED(hr)) {
							system_monitoring_interface::active_mon->log_warning("Could not create linear gradient brush ");
						}
					}
				}
			}
			return SUCCEEDED(hr);
		}

		virtual ID2D1Brush* getBrush()
		{
			return asset;
		}

		virtual std::string get_string() override
		{
			std::string value = std::format("linear:start({0},{1}),stop({2},{3})", 
				start.x, start.y, stop.x, stop.y);

			for (size_t i = 0; i < stops.size(); i++) {
				value += std::format(",stop[{0}]:pos={1},rgba({2},{3},{4},{5})",
					i, stops[i].position, 
					stops[i].color.r, stops[i].color.g, 
					stops[i].color.b, stops[i].color.a);
			}

			return value;
		}

	};

	class radialGradientBrush : public deviceDependentAsset<ID2D1RadialGradientBrush*>, brush {
	public:
		std::vector<D2D1_GRADIENT_STOP> stops;
		D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES radialProperties;

		bool create(direct2dContextBase* ptarget)
		{
			ID2D1GradientStopCollection* pGradientStops = NULL;

			HRESULT hr = HRESULT_FROM_WIN32(ERROR_BAD_ENVIRONMENT);

			if (ptarget)
			{
				if (!ptarget->getDeviceContext())
					return false;

				if (stops.size() > 0) {
					hr = ptarget->getDeviceContext()->CreateGradientStopCollection(&stops[0], stops.size(), &pGradientStops);

					if (SUCCEEDED(hr))
					{
						if (asset) {
							asset->Release();
							asset = nullptr;
						}
						hr = ptarget->getDeviceContext()->CreateRadialGradientBrush(
							radialProperties,
							D2D1::BrushProperties(),
							pGradientStops,
							&asset
						);
						pGradientStops->Release();
					}
				}
			}
			return SUCCEEDED(hr);
		}

		virtual ID2D1Brush* getBrush()
		{
			return asset;
		}

		virtual std::string get_string() override
		{
			std::string value = std::format("radial:center({0},{1}),offset({2},{3}),radius({4},{5})", 
				radialProperties.center.x, radialProperties.center.y,
				radialProperties.gradientOriginOffset.x, radialProperties.gradientOriginOffset.y,
				radialProperties.radiusX, radialProperties.radiusY);

			for (size_t i = 0; i < stops.size(); i++) {
				value += std::format(",stop[{0}]:pos={1},rgba({2},{3},{4},{5})",
					i, stops[i].position, 
					stops[i].color.r, stops[i].color.g, 
					stops[i].color.b, stops[i].color.a);
			}

			return value;
		}


	};
}

