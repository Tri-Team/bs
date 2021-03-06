/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2017 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "name.hpp"

#include "boost-test.hpp"
#include <unordered_map>

namespace ndn {
namespace tests {

BOOST_AUTO_TEST_SUITE(TestName)

static const uint8_t TestName[] = {
        0x7,  0x14, // Name
          0x8,  0x5, // NameComponent
              0x6c,  0x6f,  0x63,  0x61,  0x6c,
          0x8,  0x3, // NameComponent
              0x6e,  0x64,  0x6e,
          0x8,  0x6, // NameComponent
              0x70,  0x72,  0x65,  0x66,  0x69,  0x78
};

const uint8_t Name1[] = {0x7,  0x7, // Name
                           0x8,  0x5, // NameComponent
                             0x6c,  0x6f,  0x63,  0x61,  0x6c};

const uint8_t Name2[] = {0x7,  0xc, // Name
                           0x8,  0x5, // NameComponent
                             0x6c,  0x6f,  0x63,  0x61,  0x6c,
                           0x8,  0x3, // NameComponent
                             0x6e,  0x64,  0x6e};

// ---- encoding, decoding ----

BOOST_AUTO_TEST_CASE(Basic)
{
  Name name("/hello/world");

  BOOST_CHECK_NO_THROW(name.at(0));
  BOOST_CHECK_NO_THROW(name.at(1));
  BOOST_CHECK_NO_THROW(name.at(-1));
  BOOST_CHECK_NO_THROW(name.at(-2));

  BOOST_CHECK_THROW(name.at(2), Name::Error);
  BOOST_CHECK_THROW(name.at(-3), Name::Error);
}

BOOST_AUTO_TEST_CASE(Encode)
{
  Name name("/local/ndn/prefix");
  const Block& wire = name.wireEncode();

  // for (auto i = wire.begin(); i != wire.end(); ++i) {
  //   if (i != wire.begin())
  //     std::cout << ", ";
  //   printHex(std::cout, *i);
  // }
  // std::cout << std::endl;

  BOOST_CHECK_EQUAL_COLLECTIONS(TestName, TestName + sizeof(TestName),
                                wire.begin(), wire.end());
}

BOOST_AUTO_TEST_CASE(Decode)
{
  Block block(TestName, sizeof(TestName));

  Name name(block);

  BOOST_CHECK_EQUAL(name.toUri(), "/local/ndn/prefix");
}

BOOST_AUTO_TEST_CASE(ZeroLengthComponent)
{
  static const uint8_t compOctets[] {0x08, 0x00};
  Block compBlock(compOctets, sizeof(compOctets));
  name::Component comp;
  BOOST_REQUIRE_NO_THROW(comp.wireDecode(compBlock));
  BOOST_CHECK_EQUAL(comp.value_size(), 0);

  static const uint8_t nameOctets[] {0x07, 0x08, 0x08, 0x01, 0x41, 0x08, 0x00, 0x08, 0x01, 0x42};
  Block nameBlock(nameOctets, sizeof(nameOctets));
  static const std::string nameUri("/A/.../B");
  Name name;
  BOOST_REQUIRE_NO_THROW(name.wireDecode(nameBlock));
  BOOST_CHECK_EQUAL(name.toUri(), nameUri);
  Block nameEncoded = name.wireEncode();
  BOOST_CHECK(nameEncoded == nameBlock);

  Name name2;
  BOOST_REQUIRE_NO_THROW(name2 = Name(nameUri));
  BOOST_CHECK_EQUAL(name2.toUri(), nameUri);
  Block name2Encoded = name2.wireEncode();
  BOOST_CHECK(name2Encoded == nameBlock);
}

BOOST_AUTO_TEST_CASE(ImplicitSha256Digest)
{
  Name n;

  static const uint8_t DIGEST[] = {0x28, 0xba, 0xd4, 0xb5, 0x27, 0x5b, 0xd3, 0x92,
                                   0xdb, 0xb6, 0x70, 0xc7, 0x5c, 0xf0, 0xb6, 0x6f,
                                   0x13, 0xf7, 0x94, 0x2b, 0x21, 0xe8, 0x0f, 0x55,
                                   0xc0, 0xe8, 0x6b, 0x37, 0x47, 0x53, 0xa5, 0x48,
                                   0x00, 0x00};

  BOOST_REQUIRE_NO_THROW(n.appendImplicitSha256Digest(DIGEST, 32));
  BOOST_REQUIRE_NO_THROW(n.appendImplicitSha256Digest(make_shared<Buffer>(DIGEST, 32)));
  BOOST_CHECK_EQUAL(n.get(0), n.get(1));

  BOOST_REQUIRE_THROW(n.appendImplicitSha256Digest(DIGEST, 34), name::Component::Error);
  BOOST_REQUIRE_THROW(n.appendImplicitSha256Digest(DIGEST, 30), name::Component::Error);

  n.append(DIGEST, 32);
  BOOST_CHECK_LT(n.get(0), n.get(2));
  BOOST_CHECK_EQUAL_COLLECTIONS(n.get(0).value_begin(), n.get(0).value_end(),
                                n.get(2).value_begin(), n.get(2).value_end());

  n.append(DIGEST + 1, 32);
  BOOST_CHECK_LT(n.get(0), n.get(3));

  n.append(DIGEST + 2, 32);
  BOOST_CHECK_LT(n.get(0), n.get(4));

  BOOST_CHECK_EQUAL(n.get(0).toUri(), "sha256digest="
                    "28bad4b5275bd392dbb670c75cf0b66f13f7942b21e80f55c0e86b374753a548");

  BOOST_CHECK_EQUAL(n.get(0).isImplicitSha256Digest(), true);
  BOOST_CHECK_EQUAL(n.get(2).isImplicitSha256Digest(), false);

  BOOST_CHECK_THROW(Name("/hello/sha256digest=hmm"), name::Component::Error);

  Name n2;
  // check canonical URI encoding (lower case)
  BOOST_CHECK_NO_THROW(n2 = Name("/hello/sha256digest="
                              "28bad4b5275bd392dbb670c75cf0b66f13f7942b21e80f55c0e86b374753a548"));
  BOOST_CHECK_EQUAL(n.get(0), n2.get(1));

  // will accept hex value in upper case too
  BOOST_CHECK_NO_THROW(n2 = Name("/hello/sha256digest="
                              "28BAD4B5275BD392DBB670C75CF0B66F13F7942B21E80F55C0E86B374753A548"));
  BOOST_CHECK_EQUAL(n.get(0), n2.get(1));

  // this is not valid sha256digest component, will be treated as generic component
  BOOST_CHECK_NO_THROW(n2 = Name("/hello/SHA256DIGEST="
                              "28BAD4B5275BD392DBB670C75CF0B66F13F7942B21E80F55C0E86B374753A548"));
  BOOST_CHECK_NE(n.get(0), n2.get(1));
  BOOST_CHECK(n2.get(1).isGeneric());
}

BOOST_AUTO_TEST_CASE(NameWithSpaces)
{
  Name name("/ hello\t/\tworld \r\n");

  BOOST_CHECK_EQUAL("/hello/world", name);
  BOOST_CHECK_THROW(Name("/hello//world"), name::Component::Error);
}

BOOST_AUTO_TEST_CASE(DeepCopy)
{
  Name n1("/hello/world");
  Name n2 = n1.deepCopy();

  BOOST_CHECK_EQUAL(n1, n2);
  BOOST_CHECK_NE(&n1.wireEncode(), &n2.wireEncode());

  EncodingBuffer buffer(1024, 0);
  n1.wireEncode(buffer);
  Name n3(buffer.block());

  BOOST_CHECK_EQUAL(n1, n3);
  BOOST_CHECK_EQUAL(n3.wireEncode().getBuffer()->size(), 1024);
  n3 = n3.deepCopy();

  BOOST_CHECK_LT(n3.wireEncode().size(), 1024);
  BOOST_CHECK_EQUAL(n3.wireEncode().getBuffer()->size(), n3.wireEncode().size());
}

// ---- iterators ----

BOOST_AUTO_TEST_CASE(ForwardIterator)
{
  name::Component comps[] {
    name::Component("A"),
    name::Component("B"),
    name::Component("C"),
    name::Component("D")
  };

  Name n0;
  BOOST_CHECK_EQUAL_COLLECTIONS(n0.begin(), n0.end(), comps, comps + 0);

  Name n4("/A/B/C/D");
  BOOST_CHECK_EQUAL_COLLECTIONS(n4.begin(), n4.end(), comps, comps + 4);
}

BOOST_AUTO_TEST_CASE(ReverseIterator)
{
  name::Component comps[] {
    name::Component("D"),
    name::Component("C"),
    name::Component("B"),
    name::Component("A")
  };

  Name n0;
  BOOST_CHECK_EQUAL_COLLECTIONS(n0.rbegin(), n0.rend(), comps, comps + 0);

  Name n4("/A/B/C/D");
  BOOST_CHECK_EQUAL_COLLECTIONS(n4.rbegin(), n4.rend(), comps, comps + 4);
}

// ---- modifiers ----

BOOST_AUTO_TEST_CASE(Append)
{
  PartialName toAppend("/and");
  PartialName toAppend1("/beyond");
  {
    Name name("/hello/world");
    BOOST_CHECK_EQUAL("/hello/world/hello/world", name.append(name));
    BOOST_CHECK_EQUAL("/hello/world/hello/world", name);
  }
  {
    Name name("/hello/world");
    BOOST_CHECK_EQUAL("/hello/world/and", name.append(toAppend));
  }
  {
    Name name("/hello/world");
    BOOST_CHECK_EQUAL("/hello/world/and/beyond", name.append(toAppend).append(toAppend1));
  }

  {
    std::vector<uint8_t> vec{1, 1, 2, 3};
    Name name("/hello");
    BOOST_CHECK_EQUAL("/hello/%01%01%02%03", name.append(vec.begin(), vec.end()));
  }
}

BOOST_AUTO_TEST_CASE(AppendsAndMultiEncode)
{
  Name name("/local");
  BOOST_CHECK_EQUAL_COLLECTIONS(name.wireEncode().begin(), name.wireEncode().end(),
                                Name1, Name1 + sizeof(Name1));

  name.append("ndn");
  BOOST_CHECK_EQUAL_COLLECTIONS(name.wireEncode().begin(), name.wireEncode().end(),
                                Name2, Name2 + sizeof(Name2));

  name.append("prefix");
  BOOST_CHECK_EQUAL_COLLECTIONS(name.wireEncode().begin(), name.wireEncode().end(),
                                TestName, TestName+sizeof(TestName));
}

BOOST_AUTO_TEST_CASE(AppendNumber)
{
  Name name;
  for (uint32_t i = 0; i < 10; i++) {
    name.appendNumber(i);
  }

  BOOST_CHECK_EQUAL(name.size(), 10);

  for (uint32_t i = 0; i < 10; i++) {
    BOOST_CHECK_EQUAL(name[i].toNumber(), i);
  }
}

BOOST_AUTO_TEST_CASE(Markers)
{
  // TestNameComponent/NamingConvention provides additional coverage for these methods,
  // including verifications of the wire format.

  Name name;
  uint64_t number;

  BOOST_REQUIRE_NO_THROW(number = name.appendSegment(30923).at(-1).toSegment());
  BOOST_CHECK_EQUAL(number, 30923);

  BOOST_REQUIRE_NO_THROW(number = name.appendSegmentOffset(589).at(-1).toSegmentOffset());
  BOOST_CHECK_EQUAL(number, 589);

  BOOST_REQUIRE_NO_THROW(number = name.appendVersion().at(-1).toVersion());

  BOOST_REQUIRE_NO_THROW(number = name.appendVersion(25912).at(-1).toVersion());
  BOOST_CHECK_EQUAL(number, 25912);

  const time::system_clock::TimePoint tp = time::system_clock::now();
  time::system_clock::TimePoint tp2;
  BOOST_REQUIRE_NO_THROW(tp2 = name.appendTimestamp(tp).at(-1).toTimestamp());
  BOOST_CHECK_LE(std::abs(time::duration_cast<time::microseconds>(tp2 - tp).count()), 1);

  BOOST_REQUIRE_NO_THROW(number = name.appendSequenceNumber(11676).at(-1).toSequenceNumber());
  BOOST_CHECK_EQUAL(number, 11676);
}

BOOST_AUTO_TEST_CASE(Clear)
{
  Name n("/A");
  BOOST_CHECK_EQUAL(n.empty(), false);

  n.clear();
  BOOST_CHECK_EQUAL(n.empty(), true);
  BOOST_CHECK_EQUAL(n.size(), 0);
}

// ---- algorithms ----

BOOST_AUTO_TEST_CASE(GetSuccessor)
{
  BOOST_CHECK_EQUAL(Name("ndn:/%00%01/%01%02").getSuccessor(), Name("ndn:/%00%01/%01%03"));
  BOOST_CHECK_EQUAL(Name("ndn:/%00%01/%01%FF").getSuccessor(), Name("ndn:/%00%01/%02%00"));
  BOOST_CHECK_EQUAL(Name("ndn:/%00%01/%FF%FF").getSuccessor(), Name("ndn:/%00%01/%00%00%00"));
  BOOST_CHECK_EQUAL(Name().getSuccessor(), Name("ndn:/%00"));
}

BOOST_AUTO_TEST_CASE(Compare)
{
  BOOST_CHECK_EQUAL(Name("/A")  .compare(Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/A")  .compare(Name("/B")),   0);
  BOOST_CHECK_GT   (Name("/B")  .compare(Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/A")  .compare(Name("/AA")),  0);
  BOOST_CHECK_GT   (Name("/AA") .compare(Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/A")  .compare(Name("/A/C")), 0);
  BOOST_CHECK_GT   (Name("/A/C").compare(Name("/A")),   0);

  BOOST_CHECK_EQUAL(Name("/Z/A/Y")  .compare(1, 1, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/B")),   0);
  BOOST_CHECK_GT   (Name("/Z/B/Y")  .compare(1, 1, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/AA")),  0);
  BOOST_CHECK_GT   (Name("/Z/AA/Y") .compare(1, 1, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/A/C")), 0);
  BOOST_CHECK_GT   (Name("/Z/A/C/Y").compare(1, 2, Name("/A")),   0);

  BOOST_CHECK_EQUAL(Name("/Z/A")  .compare(1, Name::npos, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A")  .compare(1, Name::npos, Name("/B")),   0);
  BOOST_CHECK_GT   (Name("/Z/B")  .compare(1, Name::npos, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A")  .compare(1, Name::npos, Name("/AA")),  0);
  BOOST_CHECK_GT   (Name("/Z/AA") .compare(1, Name::npos, Name("/A")),   0);
  BOOST_CHECK_LT   (Name("/Z/A")  .compare(1, Name::npos, Name("/A/C")), 0);
  BOOST_CHECK_GT   (Name("/Z/A/C").compare(1, Name::npos, Name("/A")),   0);

  BOOST_CHECK_EQUAL(Name("/Z/A/Y")  .compare(1, 1, Name("/X/A/W"),   1, 1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/B/W"),   1, 1), 0);
  BOOST_CHECK_GT   (Name("/Z/B/Y")  .compare(1, 1, Name("/X/A/W"),   1, 1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/AA/W"),  1, 1), 0);
  BOOST_CHECK_GT   (Name("/Z/AA/Y") .compare(1, 1, Name("/X/A/W"),   1, 1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/A/C/W"), 1, 2), 0);
  BOOST_CHECK_GT   (Name("/Z/A/C/Y").compare(1, 2, Name("/X/A/W"),   1, 1), 0);

  BOOST_CHECK_EQUAL(Name("/Z/A/Y")  .compare(1, 1, Name("/X/A"),   1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/B"),   1), 0);
  BOOST_CHECK_GT   (Name("/Z/B/Y")  .compare(1, 1, Name("/X/A"),   1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/AA"),  1), 0);
  BOOST_CHECK_GT   (Name("/Z/AA/Y") .compare(1, 1, Name("/X/A"),   1), 0);
  BOOST_CHECK_LT   (Name("/Z/A/Y")  .compare(1, 1, Name("/X/A/C"), 1), 0);
  BOOST_CHECK_GT   (Name("/Z/A/C/Y").compare(1, 2, Name("/X/A"),   1), 0);
}

BOOST_AUTO_TEST_CASE(SubName)
{
  Name name("/hello/world");

  BOOST_CHECK_EQUAL("/hello/world", name.getSubName(0));
  BOOST_CHECK_EQUAL("/world", name.getSubName(1));
  BOOST_CHECK_EQUAL("/hello/", name.getSubName(0, 1));
}

BOOST_AUTO_TEST_CASE(SubNameNegativeIndex)
{
  Name name("/first/second/third/last");

  BOOST_CHECK_EQUAL("/last", name.getSubName(-1));
  BOOST_CHECK_EQUAL("/third/last", name.getSubName(-2));
  BOOST_CHECK_EQUAL("/second", name.getSubName(-3, 1));
}

BOOST_AUTO_TEST_CASE(SubNameOutOfRangeIndexes)
{
  Name name("/first/second/last");
  // No length
  BOOST_CHECK_EQUAL("/first/second/last", name.getSubName(-10));
  BOOST_CHECK_EQUAL("/", name.getSubName(10));

  // Starting after the max position
  BOOST_CHECK_EQUAL("/", name.getSubName(10, 1));
  BOOST_CHECK_EQUAL("/", name.getSubName(10, 10));

  // Not enough components
  BOOST_CHECK_EQUAL("/second/last", name.getSubName(1, 10));
  BOOST_CHECK_EQUAL("/last", name.getSubName(-1, 10));

  // Start before first
  BOOST_CHECK_EQUAL("/first/second", name.getSubName(-10, 2));
  BOOST_CHECK_EQUAL("/first/second/last", name.getSubName(-10, 10));
}

BOOST_AUTO_TEST_CASE(UnorderedMap)
{
  std::unordered_map<Name, int> map;
  Name name1("/1");
  Name name2("/2");
  Name name3("/3");
  map[name1] = 1;
  map[name2] = 2;
  map[name3] = 3;

  BOOST_CHECK_EQUAL(map[name1], 1);
  BOOST_CHECK_EQUAL(map[name2], 2);
  BOOST_CHECK_EQUAL(map[name3], 3);
}

BOOST_AUTO_TEST_SUITE_END() // TestName

} // namespace tests
} // namespace ndn
