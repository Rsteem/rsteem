#include <boost/test/unit_test.hpp>

#include "worker_fixture.hpp"
#include "helpers.hpp"

#include <golos/protocol/worker_operations.hpp>
#include <golos/chain/worker_objects.hpp>

using namespace golos;
using namespace golos::protocol;
using namespace golos::chain;

BOOST_FIXTURE_TEST_SUITE(worker_techspec_tests, worker_fixture)

BOOST_AUTO_TEST_CASE(worker_authorities) {
    BOOST_TEST_MESSAGE("Testing: worker_authorities");

    {
        worker_techspec_operation op;
        op.author = "bob";
        op.permlink = "bob-techspec";
        op.worker_proposal_author = "alice";
        op.worker_proposal_permlink = "alice-proposal";
        op.specification_cost = ASSET_GOLOS(6000);
        op.development_cost = ASSET_GOLOS(60000);
        op.payments_interval = 60;
        op.payments_count = 2;
        CHECK_OP_AUTHS(op, account_name_set(), account_name_set(), account_name_set({"bob"}));
    }

    {
        worker_techspec_delete_operation op;
        op.author = "bob";
        op.permlink = "bob-techspec";
        CHECK_OP_AUTHS(op, account_name_set(), account_name_set(), account_name_set({"bob"}));
    }

    {
        worker_techspec_approve_operation op;
        op.approver = "cyberfounder";
        op.author = "bob";
        op.permlink = "bob-techspec";
        op.state = worker_techspec_approve_state::approve;
        CHECK_OP_AUTHS(op, account_name_set(), account_name_set(), account_name_set({"cyberfounder"}));
    }

    {
        worker_assign_operation op;
        op.assigner = "bob";
        op.worker_techspec_author = "bob";
        op.worker_techspec_permlink = "bob-techspec";
        op.worker = "alice";
        CHECK_OP_AUTHS(op, account_name_set(), account_name_set(), account_name_set({"bob"}));

        op.worker = "";
        CHECK_OP_AUTHS(op, account_name_set(), account_name_set(), account_name_set({"bob"}));
    }
}

BOOST_AUTO_TEST_CASE(worker_techspec_validate) {
    BOOST_TEST_MESSAGE("Testing: worker_techspec_validate");

    BOOST_TEST_MESSAGE("-- Normal case");

    worker_techspec_operation op;
    op.author = "bob";
    op.permlink = "techspec-permlink";
    op.worker_proposal_author = "alice";
    op.worker_proposal_permlink = "proposal-permlink";
    op.specification_cost = ASSET_GOLOS(6000);
    op.development_cost = ASSET_GOLOS(60000);
    op.payments_interval = 60*60*24;
    op.payments_count = 2;
    CHECK_OP_VALID(op);

    BOOST_TEST_MESSAGE("-- Incorrect account or permlink case");

    CHECK_PARAM_INVALID(op, author, "");
    CHECK_PARAM_INVALID(op, permlink, std::string(STEEMIT_MAX_PERMLINK_LENGTH+1, ' '));
    CHECK_PARAM_INVALID(op, worker_proposal_author, "");
    CHECK_PARAM_INVALID(op, worker_proposal_permlink, std::string(STEEMIT_MAX_PERMLINK_LENGTH+1, ' '));

    BOOST_TEST_MESSAGE("-- Non-GOLOS cost case");

    CHECK_PARAM_INVALID(op, specification_cost, ASSET_GBG(6000));
    CHECK_PARAM_INVALID(op, specification_cost, ASSET_GESTS(6000));
    CHECK_PARAM_INVALID(op, development_cost, ASSET_GBG(60000));
    CHECK_PARAM_INVALID(op, development_cost, ASSET_GESTS(60000));

    BOOST_TEST_MESSAGE("-- Negative cost case");

    CHECK_PARAM_INVALID(op, specification_cost, ASSET_GOLOS(-1));
    CHECK_PARAM_INVALID(op, development_cost, ASSET_GOLOS(-1));

    BOOST_TEST_MESSAGE("-- Zero payments count case");

    CHECK_PARAM_INVALID(op, payments_count, 0);

    BOOST_TEST_MESSAGE("-- Too low payments interval case");

    CHECK_PARAM_INVALID(op, payments_interval, 60*60*24 - 1);

    BOOST_TEST_MESSAGE("-- Single payment with too big interval case");

    op.payments_count = 1;
    CHECK_PARAM_INVALID(op, payments_interval, 60*60*24 + 1);

    BOOST_TEST_MESSAGE("-- Single payment with normal interval case");

    op.payments_count = 1;
    CHECK_PARAM_VALID(op, payments_interval, 60*60*24);
}

BOOST_AUTO_TEST_CASE(worker_techspec_apply_create) {
    BOOST_TEST_MESSAGE("Testing: worker_techspec_apply_create");

    ACTORS((alice)(bob)(carol)(dave)(eve)(fred))
    auto private_key = create_approvers(0, STEEMIT_MAJOR_VOTED_WITNESSES);
    generate_block();

    signed_transaction tx;

    BOOST_TEST_MESSAGE("-- Create worker techspec with no post case");

    worker_techspec_operation op;
    op.author = "bob";
    op.permlink = "bob-techspec";
    op.worker_proposal_author = "alice";
    op.worker_proposal_permlink = "alice-proposal";
    op.specification_cost = ASSET_GOLOS(6);
    op.development_cost = ASSET_GOLOS(60);
    op.payments_interval = 60*60*24*2;
    op.payments_count = 2;
    GOLOS_CHECK_ERROR_MISSING(comment, make_comment_id("bob", "bob-techspec"), bob_private_key, op);
    generate_block();

    BOOST_TEST_MESSAGE("-- Create worker techspec on comment instead of post case");

    comment_create("alice", alice_private_key, "alice-proposal", "", "alice-proposal");

    comment_create("carol", carol_private_key, "i-am-comment", "alice", "alice-proposal");

    op.author = "carol";
    op.permlink = "i-am-comment";
    GOLOS_CHECK_ERROR_LOGIC(worker_techspec_can_be_created_only_on_post, carol_private_key, op);
    generate_block();

    BOOST_TEST_MESSAGE("-- Create worker techspec for non-existant proposal");

    comment_create("bob", bob_private_key, "bob-techspec", "", "bob-techspec");

    op.author = "bob";
    op.permlink = "bob-techspec";
    GOLOS_CHECK_ERROR_LOGIC(worker_techspec_can_be_created_only_for_existing_proposal, bob_private_key, op);
    generate_block();

    BOOST_TEST_MESSAGE("-- Create worker techspec for premade_work proposal");

    comment_create("dave", dave_private_key, "dave-proposal", "", "dave-proposal");

    worker_proposal("dave", dave_private_key, "dave-proposal", worker_proposal_type::premade_work);
    generate_block();

    op.worker_proposal_author = "dave";
    op.worker_proposal_permlink = "dave-proposal";
    GOLOS_CHECK_ERROR_LOGIC(cannot_create_techspec_for_premade_worker_proposal, bob_private_key, op);
    generate_block();

    BOOST_TEST_MESSAGE("-- Create worker techspec for worker proposal with approved techspec");

    {
        comment_create("eve", eve_private_key, "eve-proposal", "", "eve-proposal");

        worker_proposal("eve", eve_private_key, "eve-proposal", worker_proposal_type::task);
        generate_block();

        comment_create("fred", fred_private_key, "fred-techspec", "", "fred-techspec");

        op.author = "fred";
        op.permlink = "fred-techspec";
        op.worker_proposal_author = "eve";
        op.worker_proposal_permlink = "eve-proposal";
        BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, fred_private_key, op));

        generate_blocks(STEEMIT_MAX_WITNESSES); // Enough for approvers to reach TOP-19 and not leave it

        for (auto i = 0; i < STEEMIT_MAJOR_VOTED_WITNESSES; ++i) {
            const auto name = "approver" + std::to_string(i);

            worker_techspec_approve_operation wtaop;
            wtaop.approver = name;
            wtaop.author = "fred";
            wtaop.permlink = "fred-techspec";
            wtaop.state = worker_techspec_approve_state::approve;
            BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, private_key, wtaop));
            generate_block();
        }

        op.author = "bob";
        op.permlink = "bob-techspec";
        op.worker_proposal_author = "eve";
        op.worker_proposal_permlink = "eve-proposal";
        GOLOS_CHECK_ERROR_LOGIC(this_worker_proposal_already_has_approved_techspec, bob_private_key, op);
        generate_block();
    }

    BOOST_TEST_MESSAGE("-- Normal create worker techspec case");

    worker_proposal("alice", alice_private_key, "alice-proposal", worker_proposal_type::task);
    generate_block();

    op.worker_proposal_author = "alice";
    op.worker_proposal_permlink = "alice-proposal";
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, bob_private_key, op));
    generate_block();

    const auto& wpo_post = db->get_comment("alice", string("alice-proposal"));
    const auto& wto_post = db->get_comment("bob", string("bob-techspec"));
    const auto& wto = db->get_worker_techspec(wto_post.id);
    BOOST_CHECK_EQUAL(wto.post, wto_post.id);
    BOOST_CHECK_EQUAL(wto.worker_proposal_post, wpo_post.id);
    BOOST_CHECK(wto.state == worker_techspec_state::created);
    BOOST_CHECK_EQUAL(wto.specification_cost, op.specification_cost);
    BOOST_CHECK_EQUAL(wto.development_cost, op.development_cost);
    BOOST_CHECK_EQUAL(wto.payments_count, op.payments_count);
    BOOST_CHECK_EQUAL(wto.payments_interval, op.payments_interval);

    BOOST_CHECK_EQUAL(wto.worker, account_name_type());
    BOOST_CHECK_EQUAL(wto.worker_result_post, comment_id_type());
    BOOST_CHECK_EQUAL(wto.next_cashout_time, fc::time_point_sec::maximum());
    BOOST_CHECK_EQUAL(wto.finished_payments_count, 0);

    validate_database();
}

BOOST_AUTO_TEST_CASE(worker_techspec_apply_modify) {
    BOOST_TEST_MESSAGE("Testing: worker_techspec_apply_modify");

    ACTORS((alice)(bob)(carol))
    generate_block();

    signed_transaction tx;

    comment_create("alice", alice_private_key, "alice-proposal", "", "alice-proposal");

    worker_proposal("alice", alice_private_key, "alice-proposal", worker_proposal_type::task);
    generate_block();

    comment_create("carol", carol_private_key, "carol-proposal", "", "carol-proposal");

    worker_proposal("carol", carol_private_key, "carol-proposal", worker_proposal_type::task);
    generate_block();

    comment_create("bob", bob_private_key, "bob-techspec", "", "bob-techspec");

    worker_techspec_operation op;
    op.author = "bob";
    op.permlink = "bob-techspec";
    op.worker_proposal_author = "alice";
    op.worker_proposal_permlink = "alice-proposal";
    op.specification_cost = ASSET_GOLOS(6);
    op.development_cost = ASSET_GOLOS(60);
    op.payments_interval = 60*60*24*2;
    op.payments_count = 2;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, bob_private_key, op));
    generate_block();

    BOOST_TEST_MESSAGE("-- Trying to use worker techspec for two proposals case");

    op.worker_proposal_author = "carol";
    op.worker_proposal_permlink = "carol-proposal";
    GOLOS_CHECK_ERROR_LOGIC(this_worker_techspec_is_already_used_for_another_worker_proposal, bob_private_key, op);
    generate_block();

    BOOST_TEST_MESSAGE("-- Modify payments_count and payments_interval");

    op.worker_proposal_author = "alice";
    op.worker_proposal_permlink = "alice-proposal";
    op.payments_interval = 60*60*24*2;
    op.payments_count = 2;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, bob_private_key, op));
    generate_block();

    {
        const auto& wto_post = db->get_comment("bob", string("bob-techspec"));
        const auto& wto = db->get_worker_techspec(wto_post.id);
        BOOST_CHECK_EQUAL(wto.payments_count, op.payments_count);
        BOOST_CHECK_EQUAL(wto.payments_interval, op.payments_interval);
    }

    BOOST_TEST_MESSAGE("-- Modify payments_count and payments_interval");

    op.worker_proposal_author = "alice";
    op.worker_proposal_permlink = "alice-proposal";
    op.specification_cost = ASSET_GOLOS(7);
    op.development_cost = ASSET_GOLOS(70);
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, bob_private_key, op));
    generate_block();

    {
        const auto& wto_post = db->get_comment("bob", string("bob-techspec"));
        const auto& wto = db->get_worker_techspec(wto_post.id);
        BOOST_CHECK_EQUAL(wto.specification_cost, op.specification_cost);
        BOOST_CHECK_EQUAL(wto.development_cost, op.development_cost);
    }

    validate_database();
}

BOOST_AUTO_TEST_CASE(worker_techspec_approve_validate) {
    BOOST_TEST_MESSAGE("Testing: worker_techspec_approve_validate");

    BOOST_TEST_MESSAGE("-- Normal case");

    worker_techspec_approve_operation op;
    op.approver = "cyberfounder";
    op.author = "bob";
    op.permlink = "techspec-permlink";
    op.state = worker_techspec_approve_state::approve;
    CHECK_OP_VALID(op);

    BOOST_TEST_MESSAGE("-- Incorrect account or permlink case");

    CHECK_PARAM_INVALID(op, approver, "");
    CHECK_PARAM_INVALID(op, author, "");
    CHECK_PARAM_INVALID(op, permlink, std::string(STEEMIT_MAX_PERMLINK_LENGTH+1, ' '));

    BOOST_TEST_MESSAGE("-- Invalid state case");

    CHECK_PARAM_INVALID(op, state, worker_techspec_approve_state::_size);
}

BOOST_AUTO_TEST_CASE(worker_techspec_approve_apply_combinations) {
    BOOST_TEST_MESSAGE("Testing: worker_techspec_approve_apply_combinations");

    ACTORS((alice)(bob))
    auto private_key = create_approvers(0, 1);
    generate_block();

    signed_transaction tx;

    comment_create("alice", alice_private_key, "alice-proposal", "", "alice-proposal");

    worker_proposal("alice", alice_private_key, "alice-proposal", worker_proposal_type::task);
    generate_block();

    comment_create("bob", bob_private_key, "bob-techspec", "", "bob-techspec");

    worker_techspec_operation wtop;
    wtop.author = "bob";
    wtop.permlink = "bob-techspec";
    wtop.worker_proposal_author = "alice";
    wtop.worker_proposal_permlink = "alice-proposal";
    wtop.specification_cost = ASSET_GOLOS(6);
    wtop.development_cost = ASSET_GOLOS(60);
    wtop.payments_interval = 60*60*24*2;
    wtop.payments_count = 2;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, bob_private_key, wtop));
    generate_block();

    generate_blocks(STEEMIT_MAX_WITNESSES); // Enough for approvers to reach TOP-19 and not leave it

    BOOST_TEST_MESSAGE("-- Abstaining non-voted techspec case");

    worker_techspec_approve_operation op;
    op.approver = "approver0";
    op.author = "bob";
    op.permlink = "bob-techspec";
    op.state = worker_techspec_approve_state::abstain;
    GOLOS_CHECK_ERROR_LOGIC(you_already_have_voted_for_this_object_with_this_state, private_key, op);

    auto check_approves = [&](int approve_count, int disapprove_count) {
        auto approves = db->count_worker_techspec_approves(db->get_comment("bob", string("bob-techspec")).id);
        BOOST_CHECK_EQUAL(approves[worker_techspec_approve_state::approve], approve_count);
        BOOST_CHECK_EQUAL(approves[worker_techspec_approve_state::disapprove], disapprove_count);
    };

    BOOST_TEST_MESSAGE("-- Approving techspec (after abstain)");

    check_approves(0, 0);

    op.state = worker_techspec_approve_state::approve;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, private_key, op));
    generate_block();

    check_approves(1, 0);

    BOOST_TEST_MESSAGE("-- Repeating approve techspec case");

    GOLOS_CHECK_ERROR_LOGIC(you_already_have_voted_for_this_object_with_this_state, private_key, op);

    BOOST_TEST_MESSAGE("-- Disapproving techspec (after approve)");

    op.state = worker_techspec_approve_state::disapprove;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, private_key, op));
    generate_block();

    check_approves(0, 1);

    BOOST_TEST_MESSAGE("-- Repeating disapprove techspec case");

    GOLOS_CHECK_ERROR_LOGIC(you_already_have_voted_for_this_object_with_this_state, private_key, op);

    BOOST_TEST_MESSAGE("-- Approving techspec (after disapprove)");

    op.state = worker_techspec_approve_state::approve;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, private_key, op));
    generate_block();

    check_approves(1, 0);

    BOOST_TEST_MESSAGE("-- Abstaining techspec (after approve)");

    op.state = worker_techspec_approve_state::abstain;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, private_key, op));
    generate_block();

    check_approves(0, 0);

    BOOST_TEST_MESSAGE("-- Disapproving techspec (after abstain)");

    op.state = worker_techspec_approve_state::disapprove;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, private_key, op));
    generate_block();

    check_approves(0, 1);

    BOOST_TEST_MESSAGE("-- Abstaining techspec (after disapprove)");

    op.state = worker_techspec_approve_state::abstain;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, private_key, op));
    generate_block();

    check_approves(0, 0);
}

BOOST_AUTO_TEST_CASE(worker_techspec_approve_apply_approve) {
    BOOST_TEST_MESSAGE("Testing: worker_techspec_approve_apply_approve");

    ACTORS((alice)(bob)(carol))
    auto private_key = create_approvers(0, STEEMIT_MAJOR_VOTED_WITNESSES + 1);
    generate_block();

    signed_transaction tx;

    comment_create("alice", alice_private_key, "alice-proposal", "", "alice-proposal");

    worker_proposal("alice", alice_private_key, "alice-proposal", worker_proposal_type::task);
    generate_block();

    BOOST_TEST_MESSAGE("-- Approving techspec by not witness case");

    worker_techspec_approve_operation op;
    op.approver = "alice";
    op.author = "bob";
    op.permlink = "bob-techspec";
    op.state = worker_techspec_approve_state::approve;
    GOLOS_CHECK_ERROR_MISSING(witness, "alice", alice_private_key, op);

    BOOST_TEST_MESSAGE("-- Approving techspec by witness not in TOP-19 case");

    op.approver = "approver0";
    GOLOS_CHECK_ERROR_LOGIC(approver_of_techspec_should_be_in_top19_of_witnesses, private_key, op);

    generate_blocks(STEEMIT_MAX_WITNESSES); // Enough for approvers to reach TOP-19 and not leave it

    BOOST_TEST_MESSAGE("-- Approving techspec without post case");

    GOLOS_CHECK_ERROR_MISSING(comment, make_comment_id("bob", "bob-techspec"), private_key, op);

    BOOST_TEST_MESSAGE("-- Approving non-existing techspec case");

    comment_create("bob", bob_private_key, "bob-techspec", "", "bob-techspec");

    GOLOS_CHECK_ERROR_MISSING(worker_techspec_object, make_comment_id("bob", "bob-techspec"), private_key, op);

    worker_techspec_operation wtop;
    wtop.author = "bob";
    wtop.permlink = "bob-techspec";
    wtop.worker_proposal_author = "alice";
    wtop.worker_proposal_permlink = "alice-proposal";
    wtop.specification_cost = ASSET_GOLOS(6);
    wtop.development_cost = ASSET_GOLOS(60);
    wtop.payments_interval = 60*60*24;
    wtop.payments_count = 40;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, bob_private_key, wtop));
    generate_block();

    comment_create("carol", carol_private_key, "carol-techspec", "", "carol-techspec");

    wtop.author = "carol";
    wtop.permlink = "carol-techspec";
    wtop.specification_cost = ASSET_GOLOS(0);
    wtop.development_cost = ASSET_GOLOS(0);
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, carol_private_key, wtop));
    generate_block();

    BOOST_TEST_MESSAGE("-- Disapproving worker techspec by 1 witness");
    op.approver = "approver" + std::to_string(STEEMIT_MAJOR_VOTED_WITNESSES);
    op.state = worker_techspec_approve_state::disapprove;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, private_key, op));
    generate_block();

    BOOST_TEST_MESSAGE("-- Approving worker techspec by another witnesses");

    for (auto i = 0; i < STEEMIT_MAJOR_VOTED_WITNESSES; ++i) {
        const auto& wto = db->get_worker_techspec(db->get_comment("bob", string("bob-techspec")).id);
        BOOST_CHECK(wto.state == worker_techspec_state::created);

        const auto name = "approver" + std::to_string(i);
        op.approver = name;
        op.state = worker_techspec_approve_state::approve;
        BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, private_key, op));
        generate_block();
    }

    const auto& wto_post = db->get_comment("bob", string("bob-techspec"));
    const auto& wto = db->get_worker_techspec(wto_post.id);
    BOOST_CHECK(wto.state == worker_techspec_state::approved);

    const auto& wpo = db->get_worker_proposal(wto.worker_proposal_post);
    BOOST_CHECK_EQUAL(wpo.approved_techspec_post, wto_post.id);
    BOOST_CHECK(wpo.state == worker_proposal_state::techspec);

    auto secs = wto.payments_interval * wto.payments_count;
    auto cost = wto.specification_cost + wto.development_cost;
    auto consumption = std::min(cost * 60*60*24 / secs, cost);
    BOOST_CHECK_EQUAL(db->get_dynamic_global_properties().worker_consumption_per_day, consumption);

    BOOST_TEST_MESSAGE("-- Checking approves (they are not deleted since clear is off");

    auto checked_approves = 0;
    auto checked_disapproves = 0;
    const auto& wtao_idx = db->get_index<worker_techspec_approve_index, by_techspec_approver>();
    auto wtao_itr = wtao_idx.lower_bound(wto_post.id);
    for (; wtao_itr != wtao_idx.end(); ++wtao_itr) {
       if (wtao_itr->state == worker_techspec_approve_state::approve) {
           checked_approves++;
           continue;
       }
       checked_disapproves++;
    }
    BOOST_CHECK_EQUAL(checked_approves, STEEMIT_MAJOR_VOTED_WITNESSES);
    BOOST_CHECK_EQUAL(checked_disapproves, 1);

    BOOST_TEST_MESSAGE("-- Checking cannot approve another techspec for same worker proposal");

    op.author = "carol";
    op.permlink = "carol-techspec";
    GOLOS_CHECK_ERROR_LOGIC(this_worker_proposal_already_has_approved_techspec, private_key, op);
}

BOOST_AUTO_TEST_CASE(worker_techspec_approve_apply_disapprove) {
    BOOST_TEST_MESSAGE("Testing: worker_techspec_approve_apply_disapprove");

    ACTORS((alice)(bob)(carol))
    auto private_key = create_approvers(0, STEEMIT_SUPER_MAJOR_VOTED_WITNESSES);
    generate_block();

    signed_transaction tx;

    comment_create("alice", alice_private_key, "alice-proposal", "", "alice-proposal");
    generate_block();

    worker_proposal("alice", alice_private_key, "alice-proposal", worker_proposal_type::task);
    generate_block();

    comment_create("bob", bob_private_key, "bob-techspec", "", "bob-techspec");
    generate_block();

    worker_techspec_operation wtop;
    wtop.author = "bob";
    wtop.permlink = "bob-techspec";
    wtop.worker_proposal_author = "alice";
    wtop.worker_proposal_permlink = "alice-proposal";
    wtop.specification_cost = ASSET_GOLOS(6);
    wtop.development_cost = ASSET_GOLOS(60);
    wtop.payments_interval = 60*60*24;
    wtop.payments_count = 40;
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, bob_private_key, wtop));
    generate_block();

    comment_create("carol", carol_private_key, "carol-techspec", "", "carol-techspec");

    wtop.author = "carol";
    wtop.permlink = "carol-techspec";
    wtop.specification_cost = ASSET_GOLOS(0);
    wtop.development_cost = ASSET_GOLOS(0);
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, carol_private_key, wtop));
    generate_block();

    generate_blocks(STEEMIT_MAX_WITNESSES); // Enough for approvers to reach TOP-19 and not leave it

    BOOST_TEST_MESSAGE("-- Disapproving worker techspec by witnesses");

    worker_techspec_approve_operation op;

    for (auto i = 0; i < STEEMIT_SUPER_MAJOR_VOTED_WITNESSES; ++i) {
        const auto& wto = db->get_worker_techspec(db->get_comment("bob", string("bob-techspec")).id);
        BOOST_CHECK(wto.state == worker_techspec_state::created);

        const auto name = "approver" + std::to_string(i);
        op.approver = name;
        op.author = "bob";
        op.permlink = "bob-techspec";
        op.state = worker_techspec_approve_state::disapprove;
        BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, private_key, op));
        generate_block();
    }

    const auto& wto_post = db->get_comment("bob", string("bob-techspec"));
    const auto& wto = db->get_worker_techspec(wto_post.id);
    BOOST_CHECK(wto.state == worker_techspec_state::closed);

    BOOST_TEST_MESSAGE("-- Checking approves (they are not deleted since clear is off");

    auto checked_approves = 0;
    auto checked_disapproves = 0;
    const auto& wtao_idx = db->get_index<worker_techspec_approve_index, by_techspec_approver>();
    auto wtao_itr = wtao_idx.lower_bound(wto_post.id);
    for (; wtao_itr != wtao_idx.end(); ++wtao_itr) {
       if (wtao_itr->state == worker_techspec_approve_state::approve) {
           checked_approves++;
           continue;
       }
       checked_disapproves++;
    }
    BOOST_CHECK_EQUAL(checked_approves, 0);
    BOOST_CHECK_EQUAL(checked_disapproves, STEEMIT_SUPER_MAJOR_VOTED_WITNESSES);

    BOOST_TEST_MESSAGE("-- Checking cannot approve closed techspec");

    GOLOS_CHECK_ERROR_LOGIC(techspec_is_already_approved_or_closed, private_key, op);

    BOOST_TEST_MESSAGE("-- Checking can approve another techspec");

    op.author = "carol";
    op.permlink = "carol-techspec";
    BOOST_CHECK_NO_THROW(push_tx_with_ops(tx, private_key, op));
}

BOOST_AUTO_TEST_SUITE_END()