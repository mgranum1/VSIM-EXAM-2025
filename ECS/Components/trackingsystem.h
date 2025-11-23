#ifndef TRACKINGSYSTEM_H
#define TRACKINGSYSTEM_H

#include <glm/glm.hpp>
#include <vector>
#include <chrono>


namespace bbl
{

    struct Tracking;

    namespace TrackingSystem
    {

    // Tids baserte funksjoner
    bool shouldSample(const Tracking& tracking);
    void updateSampleTime(Tracking& tracking);

    // Kontroll punkt håndtering
    void addControlPoint(Tracking& tracking, const glm::vec3& position);
    void clearTrace(Tracking& tracking);

    // Lag B spline kurve
    void generateBSplineCurve(Tracking& tracking);

    // B spline funksjoner
    glm::vec3 evaluateBSpline(const glm::vec3& p0, const glm::vec3& p1,
                              const glm::vec3& p2, const glm::vec3& p3,
                              float t);

    void generateBSplineSegment(const glm::vec3& p0, const glm::vec3& p1,
                                const glm::vec3& p2, const glm::vec3& p3,
                                std::vector<glm::vec3>& curvePoints, size_t resolution);




    };

}
#endif // TRACKINGSYSTEM_H
