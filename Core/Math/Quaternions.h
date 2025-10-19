// For later use for rotation
#include <cmath>
#include <stdexcept>
#include <glm/glm.hpp>

namespace Quaterions
{
    struct Quat
    {
        float m_x, m_y, m_z, m_w;

        Quat() : m_x(0), m_y(0), m_z(0), m_w(1) {}
        Quat(float x, float y, float z, float w) : m_x(x), m_y(y), m_z(z), m_w(w) {}

        Quat(double angleRad, glm::fvec3 axis)
        {
            float len = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
            if (len > 0.0f) { axis.x /= len; axis.y /= len; axis.z /= len; }
            float half = static_cast<float>(angleRad * 0.5);
            float s = std::sin(half);
            m_w = std::cos(half);
            m_x = s * axis.x; m_y = s * axis.y; m_z = s * axis.z;
        }

        float magnitude() const
        {
            return std::sqrt(m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w);
        }

        float norm() const
        {
            return m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w;
        }

        Quat conjugate() const
        {
            return Quat(-m_x, -m_y, -m_z, m_w);
        }

        Quat inverse() const
        {
            float n = norm();
            if (n == 0.0f) throw std::runtime_error("Quat inverse of zero-length quaternion");
            Quat c = conjugate();
            return Quat(c.m_x / n, c.m_y / n, c.m_z / n, c.m_w / n);
        }

        Quat& operator*=(const Quat& r)
        {
            float x = m_x, y = m_y, z = m_z, w = m_w;
            m_w = w * r.m_w - x * r.m_x - y * r.m_y - z * r.m_z;
            m_x = w * r.m_x + x * r.m_w + y * r.m_z - z * r.m_y;
            m_y = w * r.m_y - x * r.m_z + y * r.m_w + z * r.m_x;
            m_z = w * r.m_z + x * r.m_y - y * r.m_x + z * r.m_w;
            return *this;
        }

        Quat& operator/=(float b)
        {
            if (b == 0.0f) throw std::runtime_error("Quat divide by zero");
            m_x /= b; m_y /= b; m_z /= b; m_w /= b;
            return *this;
        }

        Quat operator*(const Quat& r) const
        {
            Quat a = *this;
            a *= r;
            return a;
        }

        Quat operator/(float b) const
        {
            Quat a = *this;
            a /= b;
            return a;
        }

        static Quat operator*(Quat a, const Quat& b)
        {
            a *= b;
            return a;
        }

        static Quat operator/(Quat a, float b)
        {
            a /= b;
            return a;
        }
    };
}