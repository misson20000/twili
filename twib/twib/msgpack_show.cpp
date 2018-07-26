#include<iostream>
#include<tuple>
#include<array>
#include<algorithm>
#include<functional>

#include <msgpack11.hpp>

using namespace msgpack11;

void show_impl(MsgPack const& blob, int level);

void show_null(MsgPack const& blob, int level) {
    std::cout << "nil";
}

void show_bool(MsgPack const& blob, int level) {
    std::cout << (blob.bool_value() ? "true" : "false");
}

void show_float32(MsgPack const& blob, int level) {
    std::cout << std::to_string(blob.float32_value());
}

void show_float64(MsgPack const& blob, int level) {
    std::cout << std::to_string(blob.float64_value());
}

void show_int8(MsgPack const& blob, int level) {
    std::cout << std::to_string(blob.int8_value());
}

void show_int16(MsgPack const& blob, int level) {
    std::cout << blob.int16_value();
}

void show_int32(MsgPack const& blob, int level) {
    std::cout << blob.int32_value();
}

void show_int64(MsgPack const& blob, int level) {
    std::cout << blob.int64_value();
}

void show_uint8(MsgPack const& blob, int level) {
    std::cout << std::to_string(blob.uint8_value());
}

void show_uint16(MsgPack const& blob, int level) {
    std::cout << blob.uint16_value();
}

void show_uint32(MsgPack const& blob, int level) {
    std::cout << blob.uint32_value();
}

void show_uint64(MsgPack const& blob, int level) {
    std::cout << blob.uint64_value();
}

void show_string(MsgPack const& blob, int level) {
    std::cout << blob.string_value();
}

void show_array(MsgPack const& blob, int level) {
    std::cout << "[ ";

    MsgPack::array const& elements = blob.array_items();
    std::for_each( elements.begin(), elements.end(), [level](MsgPack const& elm) {
        show_impl( elm, level );
        std::cout << ", ";
    });

    std::cout << "]";
}

void show_binary(MsgPack const& blob, int level) {
    std::cout << "[ ";

    MsgPack::binary const& elements = blob.binary_items();
    std::for_each( elements.begin(), elements.end(), [level](uint8_t const elm) {
        std::cout << std::to_string(elm) << ", ";
    });

    std::cout << "]";
}

void show_object(MsgPack const& blob, int level) {
    bool is_first = level == 0;

    MsgPack::object const& elements = blob.object_items();
    std::for_each( elements.begin(), elements.end(), [level, &is_first](std::pair< MsgPack, MsgPack > const& elm ) {
        if(!is_first)
        {
            std::cout << std::endl;
        }
        is_first = false;

        for( int i = 0; i < (2 * level) ; ++i )
        {
            std:: cout << " ";
        }
        show_impl( elm.first, level + 1 );
        std::cout << " : ";
        show_impl( elm.second, level + 1 );
    });
}

void show_extension(MsgPack const& blob, int level) {
    int8_t const info = std::get<0>(blob.extension_items());
    MsgPack::binary const& elements = std::get<1>( blob.extension_items());

    std::cout << info << " - ";
    std::for_each( elements.begin(), elements.end(), [level](uint8_t const elm) {
        std::cout << std::to_string(elm) << ",";
    });
}

void show_impl(MsgPack const& blob, int level) {
    using func_pair = std::tuple< bool (MsgPack::*)() const, std::function< void(MsgPack const&, int) > >;
    static std::array<func_pair, 17> const func_table{
        std::make_tuple(&MsgPack::is_null, show_null),
        std::make_tuple(&MsgPack::is_bool, show_bool),
        std::make_tuple(&MsgPack::is_float32, show_float32),
        std::make_tuple(&MsgPack::is_float64, show_float64),
        std::make_tuple(&MsgPack::is_int8, show_int8),
        std::make_tuple(&MsgPack::is_int16, show_int16),
        std::make_tuple(&MsgPack::is_int32, show_int32),
        std::make_tuple(&MsgPack::is_int64, show_int64),
        std::make_tuple(&MsgPack::is_uint8, show_uint8),
        std::make_tuple(&MsgPack::is_uint16, show_uint16),
        std::make_tuple(&MsgPack::is_uint32, show_uint32),
        std::make_tuple(&MsgPack::is_uint64, show_uint64),
        std::make_tuple(&MsgPack::is_string, show_string),
        std::make_tuple(&MsgPack::is_array, show_array),
        std::make_tuple(&MsgPack::is_binary, show_binary),
        std::make_tuple(&MsgPack::is_object, show_object),
        std::make_tuple(&MsgPack::is_extension, show_extension)
    };

    std::for_each(func_table.begin(), func_table.end(), [&blob,level](func_pair const& funcs) {
        auto pred_pointer = std::get<0>(funcs);
        auto show_func = std::get<1>(funcs);
        if((blob.*pred_pointer)()) {
            show_func(blob,level);
        }
    });
}

void show(MsgPack const& blob ) {
    show_impl( blob, 0 );
    std::cout << std::endl;
}
