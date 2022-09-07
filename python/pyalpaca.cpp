#include <cctype>
#include <string>
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <alpaca/alpaca.h>

namespace py = pybind11;

namespace alpaca {

namespace python {

// char c
// int8_t b
// uint8_t B
// bool ?
// int16_t h
// uint16_t H
// int32_t i
// uint32_t I
// int64_t q
// uint64_t Q
// size_t N
// float f
// double d

// string s

// vector v
// array a
// map m
// unordered_map M
// set e
// unordered_set E
// tuple t

std::vector<uint8_t> serialize(const std::string& format, const py::list &args);

std::size_t get_size(const std::string& format, std::size_t& index) {
    std::string chars{""};

    // current is an digit
    auto current = [&]() { return format[index]; };
    auto move_one = [&]() { index++; };

    while (index > format.size() || std::isdigit(current())) {
        chars += current();
        move_one();
    }
    
    return std::stoll(chars);
}

auto get_value_type_array(const std::string& format, std::size_t& index) {
    std::string result{};

    auto current = [&]() { return format[index]; };
    auto move_one = [&]() { index++; };

    // current should not be ']'
    if (current() == ']') {
        throw std::runtime_error("Expected type, instead got '>'");
    }

    std::size_t level = 1;
    while (index < format.size()) {
        // base case
        if (current() == ']') {
            if (level > 0) {
                level -= 1;
            }
            if (level == 0) {
                break;
            }
            else {
                result += current();
                move_one();
            }
        }
        else {
            if (current() == '[') {
                level += 1;
            }
            result += current();
            move_one();
        }
    }

    return result;
}

auto get_value_type_vector(const std::string& format, std::size_t& index) {
    std::string result{};

    auto current = [&]() { return format[index]; };
    auto move_one = [&]() { index++; };

    // get past '['
    move_one();

    // current should not be ']'
    if (current() == ']') {
        throw std::runtime_error("Expected type, instead got '>'");
    }

    std::size_t level = 1;
    while (index < format.size()) {
        // base case
        if (current() == ']') {
            if (level > 0) {
                level -= 1;
            }
            if (level == 0) {
                break;
            }
            else {
                result += current();
                move_one();
            }
        }
        else {
            if (current() == '[') {
                level += 1;
            }
            result += current();
            move_one();
        }
    }

    return result;
}

template <options OPTIONS, class It>
void parse_array(const std::string& format, It it, std::size_t& index, std::vector<uint8_t>& result) {
    // get past '['
    index++;

    const std::size_t input_size = py::len(py::cast<py::list>(*it));
    auto size = get_size(format, index);

    if (input_size != size) {
        throw std::runtime_error("Expected " + std::to_string(size) + " values, instead found " + std::to_string(input_size));
    }

    // No need to serialize the size

    auto value_type = get_value_type_array(format, index);

    std::string value_format_list{""};
    for (std::size_t i = 0; i < size; ++i) {
        value_format_list += value_type;
    }

    // recurse - save all values
    auto list = py::cast<py::list>(*it);
    auto serialized = serialize(value_format_list, list);

    for(auto& b: serialized) {
        result.push_back(std::move(b));
    }
}

template <options OPTIONS, class It>
void parse_vector(const std::string& format, It it, std::size_t& index, std::vector<uint8_t>& result, std::size_t& byte_index) {
    // field is a std::vector

    // serialize size
    const auto size = py::len(py::cast<py::list>(*it));
    detail::to_bytes_router<OPTIONS>(size, result, byte_index);

    auto value_type = get_value_type_vector(format, index);

    std::string value_format_list{""};
    for (std::size_t i = 0; i < size; ++i) {
        value_format_list += value_type;
    }

    // recurse - save all values
    auto list = py::cast<py::list>(*it);
    auto serialized = serialize(value_format_list, list);

    for(auto& b: serialized) {
        result.push_back(std::move(b));
    }
}

std::vector<uint8_t> serialize(const std::string& format, const py::list &args) {
    std::vector<uint8_t> result;
    std::size_t byte_index = 0;

    constexpr auto OPTIONS = alpaca::options::none;

    std::size_t index = 0;

    for(auto it = args.begin(); it != args.end(); ++it) {
        if (format[index] == '?') {
            // field is a bool
            detail::to_bytes_router<OPTIONS>(it->cast<bool>(), result, byte_index);
        }
        else if (format[index] == 'c') {
            // field is a character
            detail::to_bytes_router<OPTIONS>(it->cast<char>(), result, byte_index);
        }
        else if (format[index] == 'b') {
            // field is a int8_t
            detail::to_bytes_router<OPTIONS>(it->cast<int8_t>(), result, byte_index);
        }
        else if (format[index] == 'B') {
            // field is a uint8_t
            detail::to_bytes_router<OPTIONS>(it->cast<uint8_t>(), result, byte_index);
        }
        else if (format[index] == 'h') {
            // field is a int16_t
            detail::to_bytes_router<OPTIONS>(it->cast<int16_t>(), result, byte_index);
        }
        else if (format[index] == 'H') {
            // field is a uint16_t
            detail::to_bytes_router<OPTIONS>(it->cast<uint16_t>(), result, byte_index);
        }
        else if (format[index] == 'i') {
            // field is a int32_t
            detail::to_bytes_router<OPTIONS>(it->cast<int32_t>(), result, byte_index);
        }
        else if (format[index] == 'I') {
            // field is a uint32_t
            detail::to_bytes_router<OPTIONS>(it->cast<uint32_t>(), result, byte_index);
        }
        else if (format[index] == 'q') {
            // field is a int64_t
            detail::to_bytes_router<OPTIONS>(it->cast<int64_t>(), result, byte_index);
        }
        else if (format[index] == 'Q') {
            // field is a uint64_t
            detail::to_bytes_router<OPTIONS>(it->cast<uint64_t>(), result, byte_index);
        }
        else if (format[index] == 'f') {
            // field is a float
            detail::to_bytes_router<OPTIONS>(it->cast<float>(), result, byte_index);
        }
        else if (format[index] == 'd') {
            // field is a double
            detail::to_bytes_router<OPTIONS>(it->cast<double>(), result, byte_index);
        }
        else if (format[index] == 'N') {
            // field is a std::size_t
            detail::to_bytes_router<OPTIONS>(it->cast<std::size_t>(), result, byte_index);
        }
        else if (format[index] == 's') {
            // field is a std::string
            detail::to_bytes_router<OPTIONS>(it->cast<std::string>(), result, byte_index);
        }
        else if (format[index] == '[') {

            // either a vector or an array
            // if array, next character is a digit - size of the array
            if (std::isdigit(format[index + 1])) {
                parse_array<OPTIONS>(format, it, index, result);
            }
            else {
                parse_vector<OPTIONS>(format, it, index, result, byte_index);
            }
        }

        index += 1;
    }

    return result;
}

py::list do_serialize_list(const std::string& format, const py::list& args) {
    auto bytes = serialize(format, args);
    py::list pyresult = py::cast(bytes);
    return pyresult;
}

py::bytes do_serialize(const std::string& format, const py::list& args) {
    auto bytes = serialize(format, args);
    std::string bytestring{""};
    for (const auto& b: bytes) {
        bytestring += static_cast<char>(b);
    }
    return py::bytes(bytestring);
}

}

}

PYBIND11_MODULE(pyalpaca, m)
{
  m.def("serialize", &alpaca::python::do_serialize);
  m.def("serialize_to_list", &alpaca::python::do_serialize_list);
}