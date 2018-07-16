#include "catch.hpp"

#include <mapnik/geometry/point.hpp>
#include <mapnik/well_known_srs.hpp>

TEST_CASE("well_known_srs")
{

SECTION("lonlat -> Web Mercator precision near the Equator")
{
    auto approx = Approx::custom().epsilon(1e-15);
    auto webmercy = [](double lat) {
        mapnik::geometry::point<double> p = { 0, lat };
        mapnik::lonlat2merc(p.x, p.y);
        return p.y;
    };

    // reference values were calculated by the following python
    // program with working precision set to 50 decimal digits,
    // results rounded to 20 decimal digits
    //
    //  from mpmath import mp
    //  mp.dps = 50
    //
    //  def webmercy(lat):
    //      return 6378137 * mp.log(mp.tan(0.5 * mp.radians(lat + 90)))
    //
    //  for lat in mp.linspace(-1, 1, 21):
    //      y = lat if lat == 0.0 else webmercy(lat)
    //      with mp.workdps(20):
    //          print("CHECK(webmercy(%4s) == approx(%s));" % (lat, y))
    //
    CHECK(webmercy(-1.0) == approx(-111325.14286638509823));
    CHECK(webmercy(-0.9) == approx(-100191.66201562065217));
    CHECK(webmercy(-0.8) == approx(-89058.486416709633480));
    CHECK(webmercy(-0.7) == approx(-77925.582141069444153));
    CHECK(webmercy(-0.6) == approx(-66792.915264250963194));
    CHECK(webmercy(-0.5) == approx(-55660.451865421525958));
    CHECK(webmercy(-0.4) == approx(-44528.158026848037926));
    CHECK(webmercy(-0.3) == approx(-33395.999833380203703));
    CHECK(webmercy(-0.2) == approx(-22263.943371933852011));
    CHECK(webmercy(-0.1) == approx(-11131.954730974337477));
    CHECK(webmercy( 0.0) == approx(0.0));
    CHECK(webmercy(+0.1) == approx(11131.954730974337477));
    CHECK(webmercy(+0.2) == approx(22263.943371933852011));
    CHECK(webmercy(+0.3) == approx(33395.999833380203703));
    CHECK(webmercy(+0.4) == approx(44528.158026848037926));
    CHECK(webmercy(+0.5) == approx(55660.451865421525958));
    CHECK(webmercy(+0.6) == approx(66792.915264250963194));
    CHECK(webmercy(+0.7) == approx(77925.582141069444153));
    CHECK(webmercy(+0.8) == approx(89058.486416709633480));
    CHECK(webmercy(+0.9) == approx(100191.66201562065217));
    CHECK(webmercy(+1.0) == approx(111325.14286638509823));
}

SECTION("lonlat <-> Web Mercator round-trip precision")
{
    auto approx = Approx::custom().epsilon(1e-15);
    for (double lat = -1.0; lat <= 1.0; lat += 0.1)
    {
        mapnik::geometry::point<double> p = { 0, lat };
        mapnik::lonlat2merc(p.x, p.y);
        mapnik::merc2lonlat(p.x, p.y);
        CHECK(p.y == approx(lat));
    }
    for (double lat = -7.5; lat <= 7.5; lat += 3.0)
    {
        mapnik::geometry::point<double> p = { 0, lat };
        mapnik::lonlat2merc(p.x, p.y);
        mapnik::merc2lonlat(p.x, p.y);
        CHECK(p.y == approx(lat));
    }
    for (double lat = -85.0; lat <= 85.0; lat += 10.0)
    {
        mapnik::geometry::point<double> p = { 0, lat };
        mapnik::lonlat2merc(p.x, p.y);
        mapnik::merc2lonlat(p.x, p.y);
        CHECK(p.y == approx(lat));
    }
}

} // TEST_CASE
