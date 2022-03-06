#ifndef BOX_H
#define BOX_H
#include "Params.h"
#include <algorithm>
#include <deque>
#include <vector>

#include "../Framework/Main.h"
#include "../Framework/ParticleParameter.h"
#include "../Framework/Render.h"
#include "../Framework/Test.h"

#if defined(__APPLE__) && !defined(__IOS__)
#    include <GLUT/glut.h>
#else
#    include "GL/freeglut.h"
#endif

class Box : virtual public Test, virtual public uInfoManager {
  public:
    Box() {}

  protected:
    void create_box() {
        using namespace LiquidMonsterParams;
        // add top thorns
        {
            b2BodyDef bd;
            // bd.userData = &type_thorn;
            bd.type = b2_staticBody;
            for(int i = 0; i < k_thornNumbers; i++) {
                float32 thorn_midpos =
                    (i + 0.5) * k_thornWidth + k_playfieldLeftEdge;
                bd.position.Set(thorn_midpos, k_playfieldTopEdge);
                b2Body *body = m_world->CreateBody(&bd);
                body->SetUserData(info({t_thorn, NULL}));
                b2PolygonShape shape;

                const b2Vec2 vertices[3] = {b2Vec2(-k_thornWidth / 2, 0),
                                            b2Vec2(k_thornWidth / 2, 0),
                                            b2Vec2(0, -k_thornHeight)};
                shape.Set(vertices, 3);
                auto it = body->CreateFixture(&shape, 0.0f);
                it->SetUserData(info({t_thorn, NULL}));
            }
        }

        //=========add ground & walls ===========//
        {
            // ground
            b2BodyDef bd;
            bd.type = b2_staticBody;
            bd.position.Set(0, k_playfieldBottomEdge);

            b2Body *     body = m_world->CreateBody(&bd);
            b2ChainShape shape;
            float32 k_mid = (k_playfieldLeftEdge + k_playfieldRightEdge) / 2;
            const b2Vec2 vertices[6] = {
                b2Vec2(k_playfieldLeftEdge - k_wallThickness, -k_ground_height),
                b2Vec2(k_mid, 0),
                b2Vec2(k_playfieldRightEdge + k_wallThickness,
                       -k_ground_height),
                b2Vec2(k_playfieldRightEdge + k_wallThickness,
                       -k_groundThickness - k_ground_height),
                b2Vec2(k_mid, -k_groundThickness),
                b2Vec2(k_playfieldLeftEdge - k_wallThickness,
                       -k_groundThickness - k_ground_height)

            };
            shape.CreateLoop(vertices, 6);
            body->CreateFixture(&shape, 0.0f);
            body->SetUserData(info({t_wall, NULL}));
        }
        {
            // left and right wall
            b2BodyDef bd;
            bd.type = b2_staticBody;
            for(int i = 0; i < 2; i++) {
                b2PolygonShape shape;
                const b2Vec2   vertices[4] = {
                    b2Vec2((i == 0 ? -1 : 1) * k_wallThickness,
                           k_playfieldTopEdge),
                    b2Vec2((i == 0 ? -1 : 1) * k_wallThickness,
                           k_playfieldBottomEdge - k_groundThickness),
                    b2Vec2(0, k_playfieldBottomEdge - k_groundThickness),
                    b2Vec2(0, k_playfieldTopEdge)};
                shape.Set(vertices, 4);

                bd.position.Set(
                    i == 0 ? k_playfieldLeftEdge : k_playfieldRightEdge, 0);
                b2Body *body = m_world->CreateBody(&bd);

                body->CreateFixture(&shape, 0.0f);
                body->SetUserData(info({t_wall, NULL}));
            }
        }
    }
};

#endif