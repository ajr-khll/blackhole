#include <glm/glm.hpp>

constexpr double G = 6.6743e-11;
constexpr double c = 299792458.0;

struct BlackHole {
  glm::vec3 position;
  double mass;
  double r_s;

  BlackHole(glm::vec3 pos, double m)
      : position(pos), mass(m), r_s((2.0 * G * m) / (c * c)) {}

  void draw();
};
