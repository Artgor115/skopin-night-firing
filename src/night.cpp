#include "skopin/night.hpp"

#include "skopin/kiln.hpp"
#include "skopin/measures.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace skopin {

    namespace {

        void refuse(NightReport& report, const std::string& mark, const std::string& reason) {
            Refusal refusal;
            refusal.mark = mark;
            refusal.reason = reason;
            report.refused.push_back(refusal);
        }

        void burnBisque(Kiln& kiln, const std::vector<Ware>& batch, bool nextIsGlaze, NightReport& report) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }
            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }

            kiln.open();
            for (size_t i = batch.size(); i > 0; i--) {
                kiln.load(batch[i - 1]);
            }
            kiln.close();
            kiln.fire();
            kiln.open();
            for (size_t i = 0; i < batch.size(); i++) {
                kiln.unload(batch[i].mark);
                report.passed.push_back(batch[i].mark);
            }

            if (not nextIsGlaze) {
                kiln.close();
                kiln.cool();
            }
        }

        void burnGlaze(Kiln& kiln, const std::vector<Ware>& batch, NightReport& report) {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }
            for (size_t i = batch.size(); i > 0; i--) {
                kiln.load(batch[i - 1]);
            }
            kiln.close();

            if (kiln.cone() == Cone::Cold) {
                kiln.fire();
            }
            kiln.fire();
            kiln.cool();
            kiln.cool();

            kiln.open();
            for (size_t i = 0; i < batch.size(); i++) {
                kiln.unload(batch[i].mark);
                report.passed.push_back(batch[i].mark);
            }
            kiln.close();
        }

    }

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;

    std::vector<std::string> seen;
    std::vector<Ware> waiting;
    for (size_t i = 0; i < queue.size(); i++) {
        const Ware& ware = queue[i];

        bool duplicate = false;
        for (size_t j = 0; j < seen.size(); j++) {
            if (seen[j] == ware.mark) {
                duplicate = true;
            }
        }
        if (duplicate) {
            refuse(report, ware.mark, "duplicate");
            continue;
        }
        seen.push_back(ware.mark);

        if (validateWare(ware).has_value()) {
            refuse(report, ware.mark, "invalid");
            continue;
        }
        if (ware.thickness_mm > kCrackThicknessMm) {
            refuse(report, ware.mark, "thick");
            continue;
        }
        waiting.push_back(ware);
    }

    std::vector<std::vector<Ware>> batches;
    while (waiting.size() > 0) {
        std::vector<Ware> batch;
        std::vector<Ware> left;
        batch.push_back(waiting[0]);

        if (waiting[0].kind == WareKind::Sagger) {
            for (size_t i = 1; i < waiting.size(); i++) {
                left.push_back(waiting[i]);
            }
        } else {
            bool skipped = false;
            bool wall = false;
            for (size_t i = 1; i < waiting.size(); i++) {
                const Ware& next = waiting[i];

                if (wall) {
                    left.push_back(next);
                    continue;
                }
                if (next.program != waiting[0].program or next.kind == WareKind::Sagger) {
                    wall = true;
                    left.push_back(next);
                    continue;
                }

                if (fitsInKiln(batch, next) and (not skipped or next.kind == WareKind::Rush)) {
                    batch.push_back(next);
                } else {
                    skipped = true;
                    left.push_back(next);
                }
            }
        }

        batches.push_back(batch);
        waiting = left;
    }

    Kiln kiln;
    for (size_t b = 0; b < batches.size(); b++) {
        if (batches[b][0].program == Program::Bisque) {
            bool nextIsGlaze = b + 1 < batches.size() and batches[b + 1][0].program == Program::Glaze;
            burnBisque(kiln, batches[b], nextIsGlaze, report);
        } else {
            burnGlaze(kiln, batches[b], report);
        }
    }

    report.stokes = kiln.stokes();
    return report;
}

}  // namespace skopin
