
#pragma once

namespace corona
{

    class corona_object
    {
    public:

       std::string  class_name;
       int64_t      object_id;

       std::string  created_by;
       date_time    created;

       std::string  updated_by;
       date_time    updated;

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

       chest_item make_chest_item(int _quantity)
       {
           chest_item ci;

           ci.part_class = class_name;
           ci.part_id = object_id;
           ci.quantity = _quantity;

           return ci;
       }
    };

    template <typename T> class corona_object_factory
    {

        using fn_create_object = std::function<std::shared_ptr<T>(json& _src)>;

        std::map<std::string, fn_create_object> factory_map;
        lockable factory_lock;


    public:

        corona_object_factory(const corona_object_factory& _src) = default;
        corona_object_factory(corona_object_factory&& _src) = default;
        corona_object_factory& operator =(const corona_object_factory& _src) = default;
        corona_object_factory& operator =(corona_object_factory&& _src) = default;

        void register_class(std::string _class_name, fn_create_object _ctor)
        {
            scope_lock lock(factory_lock);
            factory_map.insert_or_assign(_class_name, _ctor);
        }

        std::shared_ptr<T> create_object(json ji)
        {
            scope_lock lock(factory_lock);
            std::shared_ptr<T> sp;

            if (ji.object()) {
                std::string class_name = ji[class_name_field].as_string();

                auto foundit = factory_map.find(class_name);
                if (foundit != std::end(factory_map)) {
                    sp = foundit->second(ji);
                }
            }
            return sp;
        }

        std::vector<std::shared_ptr<T>> create_array(json j)
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
                        std::string class_name = ji[class_name_field].as_string();

                        auto foundit = factory_map.find(class_name);
                        if (foundit != std::end(factory_map)) {
                            auto object = foundit->second(ji);
                            object_list.push_back(object);
                        }
                    }
                }
            }

            return object_list;
        }
    };

}
