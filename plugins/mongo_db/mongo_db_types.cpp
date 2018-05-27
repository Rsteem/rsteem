#include <golos/plugins/mongo_db/mongo_db_types.hpp>

namespace golos {
namespace plugins {
namespace mongo_db {

    void bmi_insert_or_replace(db_map& bmi, named_document doc) {
                auto it = bmi.get<hashed_idx>().find(std::make_tuple<std::string, std::string, std::string, bool>(
                    std::string(doc.collection_name),
                    std::string(doc.key), std::string(doc.keyval), bool(doc.is_removal)));
                if (it != bmi.get<hashed_idx>().end())
                    bmi.get<hashed_idx>().erase(it);
                bmi.push_back(std::move(doc));
    }

    //void bmi_merge(db_map& bmi_what, db_map& bmi_with) {
    //    for (auto it = bmi_with.begin(); it != bmi_with.end();) {
    //        // Erase if exists
    //        auto it2 = bmi_what.get<hashed_idx>().find(std::make_tuple<std::string, std::string, std::string, bool>(
    //            std::string(it->collection_name), std::string(it->key), std::string(it->keyval),
    //            bool(it->is_removal)));
    //        if (it2 != bmi_what.get<hashed_idx>().end())
    //            bmi_what.get<hashed_idx>().erase(it2);
    //        // Move
    //        boost::optional<named_document> doc;
    //        if (bmi_with.modify(it, [&](named_document& doc_orig) { doc = std::move(doc_orig); })) {
    //            it = bmi_with.erase(it);
    //        } else {
    //            ++it;
    //        }
    //        bmi_what.push_back(std::move(*doc));
    //    }
    //}
}
}
}