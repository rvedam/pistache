#include "gtest/gtest.h"

#include <pistache/mime.h>
#include <pistache/http.h>

using namespace Pistache::Http;
using namespace Pistache::Http::Mime;

TEST(mime_test, basic_test) {
    MediaType m1(Type::Text, Subtype::Plain);
    ASSERT_EQ(m1.top(), Type::Text);
    ASSERT_EQ(m1.sub(), Subtype::Plain);
    ASSERT_EQ(m1.suffix(), Suffix::None);
    ASSERT_EQ(m1.toString(), "text/plain");

    ASSERT_EQ(m1, MIME(Text, Plain));

    auto m2 = MIME3(Application, Xhtml, Xml);
    ASSERT_EQ(m2.toString(), "application/xhtml+xml");

    auto m3 = MIME(Text, Plain);
    ASSERT_TRUE(m3.q().isEmpty());
    m3.setQuality(Q::fromFloat(0.7));
    ASSERT_EQ(m3.q().getOrElse(Q(0)), Q(70));

    ASSERT_EQ(m3.toString(), "text/plain; q=0.7");

    auto m4 = MIME3(Application, Json, Zip);
    m4.setQuality(Q::fromFloat(0.79));

    ASSERT_EQ(m4.toString(), "application/json+zip; q=0.79");

    auto m5 = MIME(Text, Html);
    m5.setQuality(Q::fromFloat(1.0));
    m5.setParam("charset", "utf-8");
    ASSERT_EQ(m5.toString(), "text/html; q=1; charset=utf-8");
}

void parse(const char* str, std::function<void (const MediaType&)> testFunc)
{
    auto m1 = MediaType::fromString(std::string(str));
    testFunc(m1);

    auto m2 = MediaType::fromRaw(str, strlen(str));
    testFunc(m2);

    MediaType m3(str, MediaType::DoParse);
    testFunc(m3);
}

TEST(mime_test, valid_parsing_test) {
    parse("application/json", [](const MediaType &m1) {
        ASSERT_EQ(m1, MIME(Application, Json));
        ASSERT_TRUE(m1.q().isEmpty());
    });

    parse("application/xhtml+xml", [](const MediaType &m2) {
        ASSERT_EQ(m2, MediaType(Type::Application, Subtype::Xhtml, Suffix::Xml));
        ASSERT_TRUE(m2.q().isEmpty());
    });

    parse("application/json; q=0.3", [](const MediaType& m3) {
        ASSERT_EQ(m3, MIME(Application, Json));
        ASSERT_EQ(m3.q().getOrElse(Q(0)), Q::fromFloat(0.3));
    });

    parse("application/xhtml+xml; q=0.7", [](const MediaType& m4) {
        ASSERT_EQ(m4.top(), Type::Application);
        ASSERT_EQ(m4.sub(), Subtype::Xhtml);
        ASSERT_EQ(m4.suffix(), Suffix::Xml);
        ASSERT_EQ(m4.q().getOrElse(Q(0)), Q(70));
    });

    parse("application/xhtml+xml; q=0.78", [](const MediaType& m5) {
        ASSERT_EQ(m5.q().getOrElse(Q(0)), Q(78));
    });

    parse("application/vnd.adobe.flash-movie", [](const MediaType& m6) {
        ASSERT_EQ(m6.top(), Type::Application);
        ASSERT_EQ(m6.sub(), Subtype::Vendor);
        ASSERT_EQ(m6.suffix(), Suffix::None);
        ASSERT_EQ(m6.rawSub(), "vnd.adobe.flash-movie");
    });

    parse("application/vnd.mycompany.myapp-v2+json", [](const MediaType& m7) {
        ASSERT_EQ(m7.top(), Type::Application);
        ASSERT_EQ(m7.sub(), Subtype::Vendor);
        ASSERT_EQ(m7.suffix(), Suffix::Json);
        ASSERT_EQ(m7.rawSub(), "vnd.mycompany.myapp-v2");
    });

    parse("application/x-myapp-v1+json", [](const MediaType& m8) {
        ASSERT_EQ(m8.top(), Type::Application);
        ASSERT_EQ(m8.sub(), Subtype::Ext);
        ASSERT_EQ(m8.suffix(), Suffix::Json);
        ASSERT_EQ(m8.rawSub(), "x-myapp-v1");
    });

    parse("audio/x-my-codec", [](const MediaType& m9) {
        ASSERT_EQ(m9.top(), Type::Audio);
        ASSERT_EQ(m9.sub(), Subtype::Ext);
        ASSERT_EQ(m9.suffix(), Suffix::None);
        ASSERT_EQ(m9.rawSub(), "x-my-codec");
    });

    parse("text/html; charset=ISO-8859-4", [](const MediaType& m10) {
        ASSERT_EQ(m10, MIME(Text, Html));
        ASSERT_TRUE(m10.q().isEmpty());
        auto charset = m10.getParam("charset");
        ASSERT_EQ(charset.getOrElse(""), "ISO-8859-4");
    });

    parse("text/html; q=0.83; charset=ISO-8859-4", [](const MediaType& m11) {
        ASSERT_EQ(m11, MIME(Text, Html));
        ASSERT_EQ(m11.q().getOrElse(Q(0)), Q(83));
        ASSERT_EQ(m11.getParam("charset").getOrElse(""), "ISO-8859-4");
    });
}

TEST(mime_test, invalid_parsing) {
    ASSERT_THROW(MediaType::fromString("applicationjson"), HttpError);
    ASSERT_THROW(MediaType::fromString("my/json"), HttpError);

    ASSERT_THROW(MediaType::fromString("text/"), HttpError);
    ASSERT_THROW(MediaType::fromString("text/plain+"), HttpError);

    ASSERT_THROW(MediaType::fromString("video/mp4;"), HttpError);

    ASSERT_THROW(MediaType::fromString("image/png;   "), HttpError);
    ASSERT_THROW(MediaType::fromString("text/plain; q"), HttpError);
    ASSERT_THROW(MediaType::fromString("text/plain;    q"), HttpError);
    ASSERT_THROW(MediaType::fromString("application/xhtml+xml;    q=a0.2"), HttpError);
    ASSERT_THROW(MediaType::fromString("application/xhtml+xml;  q=0.2b"), HttpError);

    ASSERT_THROW(MediaType::fromString("text/html; q=0.21;"), HttpError);
    ASSERT_THROW(MediaType::fromString("text/html; q=0.21; charset"), HttpError);
    ASSERT_THROW(MediaType::fromString("text/html; q=0.21; charset="), HttpError);
    ASSERT_THROW(MediaType::fromString("text/html; q=0.21; charset=ISO-8859-4;  "), HttpError);
}
