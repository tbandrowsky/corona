/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved

MIT License

About this File
The controller here mediates windows messages.  It basically handles the dx draw frame
routes messages to the presentation, which is a container of rectangles that can 
draw themselves in directx, or, conntainers of windows controls.

Notes
A corona page is a corona controller.

For Future Consideration
*/

#pragma once

namespace corona
{

	class ui_task_result_base;

	enum scrollTypes
	{
		ScrollPageUp,
		ScrollPageDown,
		ScrollLineUp,
		ScrollLineDown,
		ThumbTrack
	};

	class controller {

	public:

		virtual bool drawFrame(std::shared_ptr<direct2dContext>& _ctx) = 0;
		virtual bool update(double _elapsedSeconds, double _totalSeconds) = 0;

		virtual void keyPress(HWND _ctrl, int _key) = 0;
		virtual void keyDown(HWND _ctrl, int _key) = 0;
		virtual void keyUp(HWND _ctrl, int _key) = 0;
		virtual void mouseMove(point* _point) = 0;
		virtual void mouseLeftDown(point* _point) = 0;
		virtual void mouseLeftUp(point* _point) = 0;
		virtual void mouseRightDown(point* _point) = 0;
		virtual void mouseRightUp(point* _point) = 0;
		virtual void mouseWheel(int _delta) = 0;
		virtual void pointSelected(point* _point, ccolor* _color) = 0;
		virtual LRESULT ncHitTest(point* _point) = 0;
		virtual void setFocus(HWND _ctrl) = 0;
		virtual void killFocus(HWND _ctrl) = 0;
		virtual bool navigationKey(int _key) = 0;
		virtual void gamePad(XINPUT_STATE new_state, XINPUT_STATE old_state) = 0;

		virtual void onHostCreated() = 0;
		virtual void onCreated() = 0;
		virtual void onCommand(HWND _ctrl) = 0;
		virtual void onTextChanged(HWND _ctrl) = 0;
		virtual void onDropDownChanged(HWND _ctrl) = 0;
		virtual void onListBoxChanged(HWND _ctrl) = 0;
		virtual void onListViewChanged(HWND _ctrl) = 0;
		virtual int onHScroll(HWND _ctrl, scrollTypes scrollType) = 0;
		virtual int onVScroll(HWND _ctrl, scrollTypes scrollType) = 0;
		virtual int onResize(const rectangle& newSize, double d2dScale) = 0;
		virtual int onSpin(HWND _ctrl, int newPosition) = 0;
		virtual int layout() = 0;

		virtual int getDefaultButtonId() = 0;
		virtual void hardwareChanged() = 0;

		virtual std::string setPresentation(json presentation) = 0;

	};

}

