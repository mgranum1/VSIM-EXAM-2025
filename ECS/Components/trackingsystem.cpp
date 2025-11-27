#include "trackingsystem.h"
#include "../../ECS/Components/Components.h"
//Task 2.5
namespace bbl
{
    namespace TrackingSystem
    {
    bool shouldSample(const Tracking& tracking)
    {
        // Enkel sjekk for å se om vi skal sample en ny posisjon til Tracking systemet
        if (!tracking.isTracking || tracking.controlPoints.empty())
            return true;

        // For å Beregne tiden siden siste sampling for å kontrollere samplingshastigheten
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - tracking.lastSampleTime).count();
        return elapsed >= (tracking.samplingInterval * 1000.0f);
    }

    void updateSampleTime(Tracking& tracking)
    {
         // Oppdaterer tidspunktet for siste sampling
        tracking.lastSampleTime = std::chrono::steady_clock::now();
    }

    void addControlPoint(Tracking& tracking, const glm::vec3& position)
    {
        // Legger til nye kontroll punkt til B spline kurven

        std::vector<glm::vec3>& controlPoints = const_cast<std::vector<glm::vec3>&>(tracking.controlPoints);
        controlPoints.push_back(position);

        // Sletter gamle kontroll punkt hvis vi overstiger max antall
        // Begrenser antall kontrollpunkter for å unngå for lang kurve
        if (controlPoints.size() > tracking.maxControlPoints)
        {
            controlPoints.erase(controlPoints.begin());
        }

        // Lager ny B spline kurve når vi har nok kontrollpunkter
        // For kubisk B-spline d = 3 trenger vi minimum 4 kontrollpunkter
        if (controlPoints.size() >= 4)
        {
            generateBSplineCurve(const_cast<Tracking&>(tracking));
        }
    }

    void clearTrace(Tracking& tracking)
    {
        // Tømmer alle sporingspunkter - både kontrollpunkter og kurvepunkter
        std::vector<glm::vec3>& controlPoints = const_cast<std::vector<glm::vec3>&>(tracking.controlPoints);
        std::vector<glm::vec3>& curvePoints = const_cast<std::vector<glm::vec3>&>(tracking.curvePoints);

        controlPoints.clear();
        curvePoints.clear();
    }

    void generateBSplineCurve(Tracking& tracking)
    {
        // Genererer hele B-spline kurven fra kontrollpunktene
        // Basert på prinsippene fra avsnitt 5.5 - B spline basisfunksjoner

        std::vector<glm::vec3>& curvePoints = const_cast<std::vector<glm::vec3>&>(tracking.curvePoints);
        curvePoints.clear();

        if (tracking.controlPoints.size() < 4) return;

        // Lager kurversegmenter mellom påfølgende kontrollpunkter
        // Dette følger den lokale support egenskapen til B splines side 75 i forelesningsnotatene
        // Hver segment påvirkes kun av 4 nabokontrollpunkter
        for (size_t i = 0; i < tracking.controlPoints.size() - 3; ++i)
        {
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
        // Genererer punkter langs et enkelt B-spline segment
        // Parameter t går fra 0 til 1 for dette segmentet
        for (size_t i = 0; i <= resolution; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(resolution);
            glm::vec3 point = evaluateBSpline(p0, p1, p2, p3, t);
            curvePoints.push_back(point);
        }
    }

    glm::vec3 evaluateBSpline(const glm::vec3& p0, const glm::vec3& p1,
                              const glm::vec3& p2, const glm::vec3& p3, float t)
    {
        // B spline basis funksjoner
        // Vi lager en kubisk B spline funksjon med grad d = 3
        // som vi ser på side 79 - 80 i forelesnings notatene

         // Beregner potenser av parameteren t for effektivitet
        float t2 = t * t;
        float t3 = t2 * t;

        // Disse tilsvarer formlene fra side 79-80 for kubiske B-splines
        float b0 = (1.0f - 3.0f * t + 3.0f * t2 - t3) / 6.0f;
        float b1 = (4.0f - 6.0f * t2 + 3.0f * t3) / 6.0f;
        float b2 = (1.0f + 3.0f * t + 3.0f * t2 - 3.0f * t3) / 6.0f;
        float b3 = t3 / 6.0f;

        // Affin kombinasjon av kontrollpunkter som beskrevet i formel 5.23 side 80 i forelesnings notatene
        return b0 * p0 + b1 * p1 + b2 * p2 + b3 * p3;
    }


    }
}
