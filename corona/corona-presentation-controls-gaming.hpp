#pragma once

namespace corona
{

	class game_session_control :
		public draw_control
	{
		std::shared_ptr<game_session> current_session;
		std::map<std::string, std::shared_ptr<image_control>> images;

	public:

		game_session_control() { 
			; 
		}

		game_session_control(const game_session_control& _src) = default;

		game_session_control(control_base* _parent, int _id) : draw_control(_parent, _id) 
		{ 
			; 
		}

		virtual ~game_session_control() { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<game_session_control>(*this);
			tv->current_session = current_session;
			return tv;
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			draw_control::get_json(_dest);
		}

		virtual void put_json(json& _src)
		{
			draw_control::put_json(_src);
		}

		virtual void create_game_assets(std::shared_ptr<direct2dContext>& _context)
		{
            for (auto map : current_session->maps) {
				for (auto piece : map->pieces) {
					std::string image_name = piece->image_name;
					if (!image_name.empty() && !images.contains(image_name)) {
                        std::shared_ptr<image_control> image = std::make_shared<image_control>(this, id_counter::next(), image_name);
                        images.insert_or_assign(image_name, image);
                    }
				}
			}

			for (auto& [name, image] : images) {
				image->create(_context, host);
            }
		}

		virtual void draw_game_frame(std::shared_ptr<direct2dContext>& _context)
		{
			for (auto m : current_session->maps) {
				for (auto p : m->pieces) {
					for (auto s : p->sprites) {

					}
				}
			}
		}

        std::shared_ptr<game_session> &get_session() 
		{ 
			return current_session; 
		}

		std::shared_ptr<game_session> set_session(std::shared_ptr<game_session>& _session) 
		{ 
			if (current_session) {
				current_session->game_running = false;
            }
			auto session = current_session;
			current_session = _session; 
			return session;
		}
	};

}
