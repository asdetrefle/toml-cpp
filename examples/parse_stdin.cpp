#include <iostream>
#include <limits>
#include "tominal/toml.h"

/**
 * A visitor for toml objects that writes to an output stream in the JSON
 * format that the toml-test suite expects.
 */
class toml_test_writer
{
  public:
    toml_test_writer(std::ostream& s) : stream_(s)
    {
        // nothing
    }

    void visit(const toml::value<std::string>& v)
    {
        stream_ << "{\"type\":\"string\",\"value\":\""
                << toml::toml_writer::escape_string(v.get()) << "\"}";
    }

    void visit(const toml::value<int64_t>& v)
    {
        stream_ << "{\"type\":\"integer\",\"value\":\"" << v.get() << "\"}";
    }

    void visit(const toml::value<double>& v)
    {
        stream_ << "{\"type\":\"float\",\"value\":\"" << v.get() << "\"}";
    }

    void visit(const toml::value<toml::local_date>& v)
    {
        stream_ << "{\"type\":\"local_date\",\"value\":\"" << v.get() << "\"}";
    }

    void visit(const toml::value<toml::local_time>& v)
    {
        stream_ << "{\"type\":\"local_time\",\"value\":\"" << v.get() << "\"}";
    }

    void visit(const toml::value<toml::local_date_time>& v)
    {
        stream_ << "{\"type\":\"local_datetime\",\"value\":\"" << v.get()
                << "\"}";
    }

    void visit(const toml::value<toml::offset_date_time>& v)
    {
        stream_ << "{\"type\":\"datetime\",\"value\":\"" << v.get() << "\"}";
    }

    void visit(const toml::value<bool>& v)
    {
        stream_ << "{\"type\":\"bool\",\"value\":\"" << v << "\"}";
    }

    void visit(const toml::array& arr)
    {
        stream_ << "{\"type\":\"array\",\"value\":[";
        auto it = arr.get().begin();
        while (it != arr.get().end())
        {
            (*it)->accept(*this);
            if (++it != arr.get().end())
                stream_ << ", ";
        }
        stream_ << "]}";
    }

    void visit(const toml::table& t)
    {
        stream_ << "{";
        auto it = t.begin();
        while (it != t.end())
        {
            stream_ << '"' << toml::toml_writer::escape_string(it->first)
                    << "\":";
            it->second->accept(*this);
            if (++it != t.end())
                stream_ << ", ";
        }
        stream_ << "}";
    }

  private:
    std::ostream& stream_;
};

int main()
{
    std::cout.precision(std::numeric_limits<double>::max_digits10);
    toml::parser p{std::cin};
    try
    {
        std::shared_ptr<toml::table> g = p.parse();
        toml_test_writer writer{std::cout};
        g->accept(writer);
        std::cout << std::endl;
    }
    catch (const toml::parse_error& ex)
    {
        std::cerr << "Parsing failed: " << ex.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Something horrible happened!" << std::endl;
        // return as if there was success so that toml-test will complain
        return 0;
    }
    return 0;
}
