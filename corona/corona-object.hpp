
#pragma once

namespace corona
{

    class corona_object
    {
    public:

       comm_bus_app_interface* bus;

       std::string  class_name;
       int64_t      object_id;

       std::string  created_by;
       date_time    created;

       std::string  updated_by;
       date_time    updated;

       corona_object() = default;
       corona_object(const corona_object& _src) = default;
       corona_object(corona_object&& _src) = default;
       corona_object& operator =(const corona_object& _src) = default;
       corona_object& operator =(corona_object&& _src) = default;

       corona_object(comm_bus_app_interface* _bus) : bus(_bus)
       {

       }

       void get_json(json& _dest)
       {
           _dest.put_member_string(class_name_field, class_name);
           _dest.put_member_i64(object_id_field, object_id);
           _dest.put_member_string("created_by", created_by);
           _dest.put_member_string("updated_by", updated_by);
           _dest.put_member("created", created);
           _dest.put_member("updated", updated);
       }

       void put_json(json& _src)
       {
           class_name = _src[class_name_field].as_string();
           object_id = _src[object_id_field].as_int64_t();
           created_by = _src["created_by"].as_string();
           updated_by = _src["updated_by"].as_string();
           created = _src["created"].as_date_time();
           updated = _src["updated"].as_date_time();
       }

       virtual std::string get_item_type()
       {
           return class_name;
       }

       virtual object_reference to_reference()
       {
           object_reference ref;

           ref.class_name = class_name;
           ref.object_id = object_id;

           return ref;
       }

       virtual chest_item to_chest_item(int _quantity)
       {
           chest_item ci;

           ci.reference.class_name = class_name;
           ci.reference.object_id = object_id;
           ci.item_type = get_item_type();
           ci.quantity = _quantity;

           return ci;
       }

       corona_client_response save(corona_instance instance)
       {
            corona_client_response response;

            json_parser jp;
            json request = jp.create_object();
            get_json(request);

            response = bus->put_object(corona_instance::local, request);
       
            return response;
       }

       bool identity_matches(corona_object& _src)
       {
           return (class_name == _src.class_name) && (object_id == _src.object_id);
       }

       virtual std::shared_ptr<corona_object> clone()
       {
           json_parser jp;
           json body = jp.create_object();
           get_json(body);
           auto p = std::make_shared<corona_object>(*this);
           p->put_json(body);
           return p;
       }

       template <typename T>
       std::shared_ptr<T> copy_as(corona_instance instance)
       {
           std::shared_ptr<T> result;
           std::shared_ptr<corona_object> obj = copy(instance);
           result = std::dynamic_pointer_cast<T>(obj);
           return result;
       }

       virtual std::shared_ptr<corona_object> copy(corona_instance instance)
       {
           std::shared_ptr<corona_object> obj;

           auto create_response = bus->create_object(instance, class_name);

           if (create_response.success) {

               json_parser jp;
               json body = jp.create_object();
               get_json(body);

               body.copy_member("object_id", create_response.data);
               body.copy_member("created", create_response.data);
               body.copy_member("created_by", create_response.data);
               body.copy_member("updated", create_response.data);
               body.copy_member("updated_by", create_response.data);

               obj = std::make_shared<corona_object>(*this);
               obj->put_json(body);
           }
           return obj;
       }
    };

    template <typename T> class function_scheduler : public corona_object
    {
    public:


        function_scheduler(double _seconds, T _value, std::function<void(T src)> _task)
            : remaining_seconds(0), frequency_seconds(_seconds), duration_seconds(_seconds), task(_task), value(_value)
        {

        }

        function_scheduler()
        {
            frequency_seconds = 1;
            remaining_seconds = 0;
            duration_seconds = 2;
            enabled = false;
            value = {};
            task = {};
        }

        function_scheduler(const function_scheduler& _src) = default;
        function_scheduler(function_scheduler&& _src) = default;
        function_scheduler& operator=(const function_scheduler& _src) = default;
        function_scheduler& operator=(function_scheduler&& _src) = default;

        std::function<void(T src)>	task;
        double						frequency_seconds;
        double						remaining_seconds;
        double						duration_seconds;
        T							value;
        bool						enabled;

        bool execute(double elapsed)
        {
            if (!enabled)
                return false;

            duration_seconds -= elapsed;
            remaining_seconds -= elapsed;

            if (duration_seconds < 0) {
                return false;
            }

            if (remaining_seconds <= 0.0)
            {
                try
                {
                    if (task)
                    {
                        task(value);
                    }
                }
                catch (std::exception exc)
                {
                }
                remaining_seconds = frequency_seconds;
                return true
            }

            return false;
        }

        void get_json(json& _dest)
        {
            corona_object::get_json(_dest);
            _dest.put_member("frequency_seconds", frequency_seconds);
            _dest.put_member("remaining_seconds", remaining_seconds);
            _dest.put_member("duration_seconds", duration_seconds);
            _dest.put_member("enabled", enabled);
        }

        void put_json(json& _src)
        {
            corona_object::put_json(_src);
            frequency_seconds = _src["frequency_seconds"].as_double();
            remaining_seconds = _src["remaining_seconds"].as_double();
            duration_seconds = _src["duration_seconds"].as_double();
            enabled = _src["enabled"].as_bool();
        }
    };

    class animation_scheduler : public corona_object
    {
    public:


        animation_scheduler()
        {
            enabled = false;
        }

        animation_scheduler(const animation_scheduler& _src) = default;
        animation_scheduler(animation_scheduler&& _src) = default;
        animation_scheduler& operator=(const animation_scheduler& _src) = default;
        animation_scheduler& operator=(animation_scheduler&& _src) = default;

        std::vector<double>			schedule;
        double						current_seconds;
        double                      total_seconds;
        bool						enabled;
        int                         current_index;

        int execute(double elapsed)
        {
            if (!enabled)
                return 0;

            if (schedule.size() > 0) {

                current_seconds = fmod(elapsed + current_seconds, total_seconds);
                double t = 0;
                while (t < current_seconds) {
                    t += schedule[current_index];
                    if (t >= current_seconds) {
                        return current_index;
                    }
                    current_index = (current_index + 1) % schedule.size();
                }

            }
            return 0;
        }

        void get_json(json& _dest)
        {
            corona_object::get_json(_dest);
            _dest.put_member("current_seconds", current_seconds);
            _dest.put_member("total_seconds", total_seconds);
            _dest.put_member("enabled", enabled);
            _dest.put_member("current_index", current_index);
            json_parser jp;
            json jschedule = jp.create_array();
            for (const auto& e : schedule) {
                jschedule.push_back(e);
            }
            _dest.put_member("schedule", jschedule);
        }

        void put_json(json& _src)
        {
            corona_object::put_json(_src);
            current_seconds = _src["current_seconds"].as_double();
            total_seconds = _src["total_seconds"].as_double();
            enabled = _src["enabled"].as_bool();
            current_index = _src["current_index"].as_int();

            schedule.clear();
            total_seconds = 0.0;

            for (auto e : _src["schedule"].as_array()) {
                schedule.push_back(e.as_double());
            }
        }
    };


    template <typename T> class corona_object_factory
    {

        using fn_create_object = std::function<std::shared_ptr<T>(json& _src, comm_bus_app_interface* _bus)>;

        std::map<std::string, fn_create_object> factory_map;
        lockable factory_lock;

        comm_bus_app_interface* bus;
        std::map<std::string, json> class_cache;

    public:

        corona_object_factory(comm_bus_app_interface* _bus) noexcept {
            bus = _bus;
        }
        corona_object_factory(const corona_object_factory& _src) = default;
        corona_object_factory(corona_object_factory&& _src) = default;
        corona_object_factory& operator =(const corona_object_factory& _src) = default;
        corona_object_factory& operator =(corona_object_factory&& _src) = default;

        void register_class(std::string _class_name, fn_create_object _ctor)
        {
            scope_lock lock(factory_lock);
            factory_map.insert_or_assign(_class_name, _ctor);
        }

        // you can do this before or after register classes
        // but, it is best to do it after, so that you can register the classes first, and then get the list of classes from the bus.
        // and someday we want to filter this so that it is just the classes and descendants 
        // of a particular item, so the corona api will have be tweaked to allow this.
        void init(corona_instance _instance)
        {
            auto result = bus->get_classes(_instance);
            if (!result.success) {
                bus->log_error(result.message, __FILE__, __LINE__);
                throw std::runtime_error(result.message);
            }

            if (result.data.array()) {
                for (int i = 0; i < result.data.size(); i++) {
                    auto jclass = result.data.get_element(i);
                    if (!jclass.object()) {
                        continue;
                    }
                    std::string class_name = jclass[class_name_field].as_string();
                    if (!class_name.empty()) {
                        class_cache[class_name] = jclass;
                    }
                }
            }
        }

        template <typename U = T>
        std::shared_ptr<U> get_object(corona_instance _instance, std::string class_name, int64_t object_id, bool include_children)
        {
            std::shared_ptr<U> result;
            json_parser jp;

            if (class_name.empty() || object_id <= 0 || !factory_map.contains(class_name)) {
                return result;
            }

            json request = jp.create_object();
            request.put_member(class_name_field, class_name);
            request.put_member_i64(object_id_field, object_id);
            request.put_member("include_children", include_children);

            auto ccr = bus->get_object(_instance, request);

            if (ccr.success) {
                result = create_object<U>(ccr.data);
            }

            return result;
        }

        template<typename U = T> 
        std::shared_ptr<U> get_object(object_reference& ref, bool _children)
        {
            auto obj = get_object<U>(corona_instance _instance, ref.class_name, ref.object_id, false);
            return obj;
        }

        template <typename U=T> 
        std::shared_ptr<U> create_object(json ji)
        {
            scope_lock lock(factory_lock);
            std::shared_ptr<U> sp;

            if (ji.object()) {
                std::string class_name = ji[class_name_field].as_string();

                auto foundit = factory_map.find(class_name);
                if (foundit != std::end(factory_map)) {
                    auto p = foundit->second(ji, bus);
                    sp = std::dynamic_pointer_cast<U>(p);
                    if (sp) {
                        if (constexpr bool is_base_of = std::is_base_of_v<corona_object, U>::value) {
                            sp->bus = bus;
                        }
                    }
                }
            }
            return sp;
        }

        template <typename U = T>
        std::shared_ptr<U> create_object(std::string class_name)
        {
            scope_lock lock(factory_lock);
            std::shared_ptr<U> sp;

            auto foundit = factory_map.find(class_name);
            if (foundit != std::end(factory_map)) {
                auto p = foundit->second(ji, bus);
                sp = std::dynamic_pointer_cast<U>(p);
                if (sp) {
                    if (constexpr bool is_base_of = std::is_base_of_v<corona_object, U>::value) {
                        sp->bus = bus;
                        auto response = _bus->create_object(instance, class_name);

                        if (response.success) {
                            sp->put_json(response.data);
                            bus->put_object(instance, response.data);
                        }
                    }
                }
            }
            return sp;
        }

        template <typename U=T>
        std::vector<std::shared_ptr<U>> create_array(json j)
        {
            scope_lock lock(factory_lock);
            std::vector<std::shared_ptr<T>> object_list;

            if (j.array()) 
            {
                for (int i = 0; i < j.size(); i++) 
                {
                    auto ji = j.get_element(i);

                    if (ji.object()) 
                    {
                        auto obj = create_object<U>(ji);
                        if (obj) {
                            object_list.push_back(obj);
                        }
                    }
                }
            }

            return object_list;
        }
    };

}
