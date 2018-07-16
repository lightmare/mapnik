#include "bench_framework.hpp"

#include <mapnik/well_known_srs.hpp>


struct test_webmerc : public benchmark::test_case
{
    int const from_;

    test_webmerc(mapnik::parameters const& params, int from)
      : test_case(params), from_(from) {}

    bool validate() const
    {
        return from_ == 3857 || from_ == 4326;
    }

    bool operator() () const
    {
        double x = 0, y = 0;
        switch (from_) {
        case 3857:
            return mapnik::merc2lonlat(&x, &y, this->iterations_, 0);
        case 4326:
            return mapnik::lonlat2merc(&x, &y, this->iterations_, 0);
        }
        return false;
    }
};


int main(int argc, char** argv)
{
    return benchmark::sequencer(argc, argv)
        .run<test_webmerc>("lonlat->merc", 4326)
        .run<test_webmerc>("merc->lonlat", 3857)
        .done();
}
