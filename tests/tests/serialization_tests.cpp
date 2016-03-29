/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/test/unit_test.hpp>

#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/database.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/variant.hpp>

#include "../common/database_fixture.hpp"

#include <cmath>

using namespace steemit::chain;

BOOST_FIXTURE_TEST_SUITE( serialization_tests, database_fixture )

BOOST_AUTO_TEST_CASE( serialization_raw_test )
{
   try {
      ACTORS( (alice)(bob) )
      transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = asset(100,STEEM_SYMBOL);

      trx.operations.push_back( op );
      auto packed = fc::raw::pack( trx );
      signed_transaction unpacked = fc::raw::unpack<signed_transaction>(packed);
      unpacked.validate();
      BOOST_CHECK( trx.digest() == unpacked.digest() );
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}
BOOST_AUTO_TEST_CASE( serialization_json_test )
{
   try {
      ACTORS( (alice)(bob) )
      transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = asset(100,STEEM_SYMBOL);


      fc::variant test(op.amount);
      auto tmp = test.as<asset>();

      trx.operations.push_back( op );
      fc::variant packed(trx);
      signed_transaction unpacked = packed.as<signed_transaction>();
      unpacked.validate();
      BOOST_CHECK( trx.digest() == unpacked.digest() );
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( asset_test )
{
   try
   {
      BOOST_CHECK_EQUAL( asset().decimals(), 3 );
      BOOST_CHECK_EQUAL( asset().symbol_name(), "STEEM" );
      BOOST_CHECK_EQUAL( asset().to_string(), "0.000 STEEM" );

      BOOST_TEST_MESSAGE( "Asset Test" );
      asset steem = asset::from_string( "123.456 STEEM" );
      asset sbd = asset::from_string( "654.321 SBD" );
      asset tmp = asset::from_string( "0.456 STEEM" );
      BOOST_CHECK_EQUAL( tmp.amount.value, 456 );
      tmp = asset::from_string( "0.056 STEEM" );
      BOOST_CHECK_EQUAL( tmp.amount.value, 56 );

      BOOST_CHECK( std::abs( steem.to_real() - 123.456 ) < 0.0005 );
      BOOST_CHECK_EQUAL( steem.decimals(), 3 );
      BOOST_CHECK_EQUAL( steem.symbol_name(), "STEEM" );
      BOOST_CHECK_EQUAL( steem.to_string(), "123.456 STEEM" );
      BOOST_CHECK_EQUAL( steem.symbol, STEEM_SYMBOL);
      BOOST_CHECK_EQUAL( asset(50, STEEM_SYMBOL).to_string(), "0.050 STEEM" );
      BOOST_CHECK_EQUAL( asset(50000, STEEM_SYMBOL).to_string(), "50.000 STEEM" );

      BOOST_CHECK( std::abs( sbd.to_real() - 654.321 ) < 0.0005 );
      BOOST_CHECK_EQUAL( sbd.decimals(), 3 );
      BOOST_CHECK_EQUAL( sbd.symbol_name(), "SBD" );
      BOOST_CHECK_EQUAL( sbd.to_string(), "654.321 SBD" );
      BOOST_CHECK_EQUAL( sbd.symbol, SBD_SYMBOL);
      BOOST_CHECK_EQUAL( asset(50, SBD_SYMBOL).to_string(), "0.050 SBD" );
      BOOST_CHECK_EQUAL( asset(50000, SBD_SYMBOL).to_string(), "50.000 SBD" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( json_tests )
{
   try {
   auto var = fc::json::variants_from_string( "10.6 " );
   var = fc::json::variants_from_string( "10.5" );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( extended_private_key_type_test )
{
   try
   {
     fc::ecc::extended_private_key key = fc::ecc::extended_private_key( fc::ecc::private_key::generate(),
                                                                       fc::sha256(),
                                                                       0, 0, 0 );
      extended_private_key_type type = extended_private_key_type( key );
      std::string packed = std::string( type );
      extended_private_key_type unpacked = extended_private_key_type( packed );
      BOOST_CHECK( type == unpacked );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( extended_public_key_type_test )
{
   try
   {
      fc::ecc::extended_public_key key = fc::ecc::extended_public_key( fc::ecc::private_key::generate().get_public_key(),
                                                                       fc::sha256(),
                                                                       0, 0, 0 );
      extended_public_key_type type = extended_public_key_type( key );
      std::string packed = std::string( type );
      extended_public_key_type unpacked = extended_public_key_type( packed );
      BOOST_CHECK( type == unpacked );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_SUITE_END()
