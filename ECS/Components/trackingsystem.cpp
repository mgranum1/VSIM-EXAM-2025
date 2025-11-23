#include "trackingsystem.h"
#include "../../ECS/Components/Components.h"

namespace bbl
{
    namespace TrackingSystem
    {
    bool shouldSample(const Tracking& tracking)
    {
        if (!tracking.isTracking || tracking.controlPoints.empty())
            return true;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - tracking.lastSampleTime).count();
        return elapsed >= (tracking.samplingInterval * 1000.0f);
    }

    void updateSampleTime(Tracking& tracking)
    {
        tracking.lastSampleTime = std::chrono::steady_clock::now();
    }

    void addControlPoint(Tracking& tracking, const glm::vec3& position)
    {
        // Legger til nye kontroll punkt
        auto& controlPoints = const_cast<std::vector<glm::vec3>&>(tracking.controlPoints);
        controlPoints.push_back(position);

        // Sletter gamle kontroll punkt hvis vi overstiger max antall
        if (controlPoints.size() > tracking.maxControlPoints) {
            controlPoints.erase(controlPoints.begin());
        }

        // Lager ny kurve hvis vi har nok punkter
        if (controlPoints.size() >= 4) {
            generateBSplineCurve(const_cast<Tracking&>(tracking));
        }
    }

    void clearTrace(Tracking& tracking)
    {
        auto& controlPoints = const_cast<std::vector<glm::vec3>&>(tracking.controlPoints);
        auto& curvePoints = const_cast<std::vector<glm::vec3>&>(tracking.curvePoints);

        controlPoints.clear();
        curvePoints.clear();
    }

    void generateBSplineCurve(Tracking& tracking)
    {
        auto& curvePoints = const_cast<std::vector<glm::vec3>&>(tracking.curvePoints);
        curvePoints.clear();

        if (tracking.controlPoints.size() < 4) return;

        // Lag kurver mellom kontroll punktene
        for (size_t i = 0; i < tracking.controlPoints.size() - 3; ++i) {
            generateBSplineSegment(
                tracking.controlPoints[i],
                tracking.controlPoints[i + 1],
                tracking.controlPoints[i + 2],
                tracking.controlPoints[i + 3],
                curvePoints,
                tracking.curveResolution
                );
        }
    }

    void generateBSplineSegment(const glm::vec3& p0, const glm::vec3& p1,
                                const glm::vec3& p2, const glm::vec3& p3,
                                std::vector<glm::vec3>& curvePoints,
                                size_t resolution)
    {
        for (size_t i = 0; i <= resolution; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(resolution);
            glm::vec3 point = evaluateBSpline(p0, p1, p2, p3, t);
            curvePoints.push_back(point);
        }
    }

    glm::vec3 evaluateBSpline(const glm::vec3& p0, const glm::vec3& p1,
                              const glm::vec3& p2, const glm::vec3& p3, float t)
    {
        // B spline basis funksjoner
        float t2 = t * t;
        float t3 = t2 * t;

        float b0 = (1.0f - 3.0f * t + 3.0f * t2 - t3) / 6.0f;
        float b1 = (4.0f - 6.0f * t2 + 3.0f * t3) / 6.0f;
        float b2 = (1.0f + 3.0f * t + 3.0f * t2 - 3.0f * t3) / 6.0f;
        float b3 = t3 / 6.0f;

        return b0 * p0 + b1 * p1 + b2 * p2 + b3 * p3;
    }


    }
}
