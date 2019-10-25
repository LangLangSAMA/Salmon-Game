#pragma once

#include "common.hpp"
#include <vector>

class Dot : public Entity
{
public:
    // Creates all the associated render resources and default transform
    bool init(int color = DOT);

    // Releases all associated resources
    void destroy();

    // Renders the water
    void draw(const mat3 &projection) override;

    void update(vec2 pos);

private:
    int m_type;
    std::vector<Vertex> m_vertices;
    std::vector<uint16_t> m_indices;
};
