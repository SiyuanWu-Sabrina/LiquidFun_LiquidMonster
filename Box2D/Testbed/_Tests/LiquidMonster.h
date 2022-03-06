#ifndef LIQUID_MONSTER_H
#define LIQUID_MONSTER_H
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

// The following parameters are not static const members of the LiquidMonster
// class with values assigned inline as it can result in link errors when using
// gcc.
namespace LiquidMonsterParams {

static const float32 k_playfieldWidth      = 40;
static const float32 k_playfieldHeight     = 55;
static const float32 k_playfieldLeftEdge   = -k_playfieldWidth / 2;
static const float32 k_playfieldRightEdge  = k_playfieldWidth / 2;
static const float32 k_playfieldBottomEdge = -5;
static const float32 k_playfieldTopEdge =
    k_playfieldBottomEdge + k_playfieldHeight;

static const int     k_thornNumbers = 15;
static const float32 k_thornWidth =
    (k_playfieldRightEdge - k_playfieldLeftEdge) / k_thornNumbers;
static const float32 k_thornHeight = 1.5;

static const float32 k_groundThickness = 2;
static const float32 k_wallThickness   = 2;

static const float32 k_floor_height = 10;

}  // namespace LiquidMonsterParams

bool b2ContactFilter::ShouldCollide(b2Fixture *fixtureA, b2Fixture *fixtureB) {
    const b2Filter &filterA = fixtureA->GetFilterData();
    const b2Filter &filterB = fixtureB->GetFilterData();
    if(filterA.groupIndex == filterB.groupIndex && filterA.groupIndex != 0) {
        return filterA.groupIndex > 0;
    }
    bool collide = (filterA.maskBits & filterB.categoryBits) != 0 &&
        (filterA.categoryBits & filterB.maskBits) != 0;
    return collide;
}

class LiquidMonster : public Test {
  public:
    int              type_thorn;
    int              type_ball;
    int              type_floor;
    int              particle_lowerbound;
    b2Body *         monster_handle;
    b2ParticleGroup *monster_particles;
    // record floors
    std::deque<std::pair<b2Body *, b2Body *>> floors;
    LiquidMonster() {
        using namespace LiquidMonsterParams;
        type_thorn = 123;
        type_ball  = 123123123;
        type_floor = 32131231;
        m_world->SetGravity({0.0, -10.0});
        // ** ----------- **
        // add top thorns
        {
            b2BodyDef bd;
            bd.userData = &type_thorn;
            bd.type     = b2_staticBody;
            for(int i = 0; i < k_thornNumbers; i++) {
                // add position to the thorn
                float32 thorn_midpos =
                    (i + 0.5) * k_thornWidth + k_playfieldLeftEdge;
                bd.position.Set(thorn_midpos, k_playfieldTopEdge);
                // add body to the thorn
                b2Body *body = m_world->CreateBody(&bd);

                // add shape to the thorn
                b2PolygonShape shape;

                const b2Vec2 vertices[3] = {b2Vec2(-k_thornWidth / 2, 0),
                                            b2Vec2(k_thornWidth / 2, 0),
                                            b2Vec2(0, -k_thornHeight)};
                shape.Set(vertices, 3);
                body->CreateFixture(&shape, 0.0f)->SetUserData(&type_thorn);
            }
        }

        //=========add ground & walls ===========//
        {
            // ground
            b2BodyDef bd;
            bd.type = b2_staticBody;
            bd.position.Set(0, k_playfieldBottomEdge);

            b2Body *body = m_world->CreateBody(&bd);

            b2PolygonShape shape;
            const b2Vec2   vertices[4] = {
                b2Vec2(k_playfieldLeftEdge - k_wallThickness, 0),
                b2Vec2(k_playfieldRightEdge + k_wallThickness, 0),
                b2Vec2(k_playfieldRightEdge + k_wallThickness,
                       -k_groundThickness),
                b2Vec2(k_playfieldLeftEdge - k_wallThickness,
                       -k_groundThickness)};
            shape.Set(vertices, 4);
            body->CreateFixture(&shape, 0.0f)->SetUserData(&type_ball);
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

                body->CreateFixture(&shape, 0.0f)->SetUserData(&type_ball);
            }
        }
        {
            // add a little ball as a handle
            b2BodyDef bd;
            bd.type             = b2_dynamicBody;
            bd.userData         = &type_ball;
            b2CircleShape shape = b2CircleShape();
            shape.m_radius      = 0.8f;
            bd.gravityScale = 0.5;

            bd.position.Set(0, 2);
            b2Body *body = m_world->CreateBody(&bd);

            body->CreateFixture(&shape, 10.0f)->SetUserData(&type_ball);
            monster_handle = body;
        }
        // a chain around liquid monster
        {
            m_particleSystem->SetDensity(0.5);
            m_particleSystem->SetRadius(0.15f);
            m_particleSystem->SetGravityScale(0.5);
            b2CircleShape shape;
            shape.m_p.Set(0, 2);
            shape.m_radius = 4.0f;

            b2ParticleGroupDef pd;
            pd.userData   = &type_ball;
            pd.flags      = b2_springParticle | b2_barrierParticle;
            pd.groupFlags = b2_solidParticleGroup;
            pd.shape      = &shape;
            pd.color.Set(0xff, 0x00, 0x00, 255);
            m_particleSystem->CreateParticleGroup(pd)->SetUserData(&type_ball);
        }
        b2CircleShape innershape;
        innershape.m_p.Set(0, 0);
        innershape.m_radius = 2.80f;
        b2Transform trans   = b2Transform();
        trans.Set(b2Vec2{0, 2}, 0);
        m_particleSystem->DestroyParticlesInShape(innershape, trans);

        // liquid monster
        {
            b2CircleShape shape;
            shape.m_p.Set(0, 2);
            shape.m_radius = 2.75f;
            b2ParticleGroupDef pd;
            pd.userData = &type_ball;
            pd.flags    = b2_tensileParticle;
            // pd.strength = 0.05f;
            // pd.groupFlags = 0;
            pd.shape = &shape;
            pd.color.Set(0x66, 0xcc, 0xff, 255);
            auto it = m_particleSystem->CreateParticleGroup(pd);
            it->SetUserData(&type_ball);
            monster_particles   = it;
            particle_lowerbound = it->GetParticleCount() / 3;
        }

        this->create_floor();
    }
    ~LiquidMonster() {
        // for(auto p : to_be_cleaned){
        //     delete p;
        // }
    }
    static Test *Create() {
        return new LiquidMonster;
    }



    void Step(Settings *settings) {
        using namespace LiquidMonsterParams;
        Test::Step(settings);
        static int i = 0;
        if(floors.front().first->GetPosition().y >
           k_playfieldTopEdge + k_thornHeight) {
            m_world->DestroyBody(floors.front().first);
            m_world->DestroyBody(floors.front().second);
            floors.pop_front();
        }
        if(floors.back().first->GetPosition().y >
           k_playfieldBottomEdge + RandomFloat(0.5,1.5) * k_floor_height) {
            this->create_floor();
        }
        i++;
        std::printf("step:%d\n", i);
        // for(b2Contact *c = m_world->GetContactList(); c; c = c->GetNext()) {
        //     std::printf("A: %d\n", *((int *)c->GetFixtureA()->GetUserData()));
        //     std::printf("B: %d\n", *((int *)c->GetFixtureB()->GetUserData()));
        // }
        // std::vector<b2ParticleGroup *> to_be_deleted;
        // for(auto g = m_particleSystem->GetParticleGroupList(); g;
        //     g      = g->GetNext()) {
        //     if(g->GetCenter().y > k_playfieldTopEdge - 5 * k_thornHeight) {
        //         to_be_deleted.push_back(g);
        //     }
        // }
        // for(auto g : to_be_deleted) {
        //     m_particleSystem->SplitParticleGroup(g);
        // }
        // printf("particles contact:\n");
        // for(int i = 0; i < m_particleSystem->GetContactCount(); i++) {
        //     auto it = m_particleSystem->GetContacts()[i];
        //     printf("  index: %d %d\n", it.GetIndexA(), it.GetIndexB());
        // }
        printf("particles contact:\n");
        for(int i = 0; i < m_particleSystem->GetBodyContactCount(); i++) {
            auto it = m_particleSystem->GetBodyContacts()[i];
            if(it.body->GetUserData() == &type_thorn)
                m_particleSystem->DestroyParticle(it.index);

            // printf("  index: %d %d\n", it.GetIndexA(), it.GetIndexB());
        }
        // m_particleSystem->DestroyParticlesInShape(m_killfieldShape,
        // m_killfieldTransform);
        {
            char score[100];        
            sprintf(score,"now particles: %d",int(monster_particles->GetParticleCount()));
            m_debugDraw.DrawString(5,m_textLine, score);
            m_textLine += DRAW_STRING_NEW_LINE;            
        }
        if(monster_particles->GetParticleCount() < particle_lowerbound) {
            m_debugDraw.DrawString(5,m_textLine, "you lose!");
            m_textLine += DRAW_STRING_NEW_LINE;
        }

    }
    void breed() {

    }
    void shoot_a_ball() {

    }
    void create_floor() {
        using namespace LiquidMonsterParams;
        float32 k_down_length     = 10.0f;
        float32 k_floor_thickness = 1.5f;
        float32 k_floor_velocity = 3.0f;
        float32 posX =
            float32(rand()) / RAND_MAX * (k_playfieldWidth - k_down_length) +
            k_playfieldLeftEdge;
        // floors
        std::pair<b2Body *, b2Body *> new_floor;
        {
            // floor
            b2BodyDef bd;
            bd.type = b2_kinematicBody;
            bd.position.Set(0, k_playfieldBottomEdge - k_wallThickness);
            {
                b2Body *lbody = m_world->CreateBody(&bd);
                lbody->SetLinearVelocity(b2Vec2(0,k_floor_velocity));
                new_floor.first = lbody;
                b2PolygonShape shape;
                const b2Vec2   lvertices[4] = {
                    b2Vec2{k_playfieldLeftEdge, 0},
                    b2Vec2{posX, 0},
                    b2Vec2{posX, -k_floor_thickness},
                    b2Vec2{k_playfieldLeftEdge, -k_floor_thickness},
                };
                shape.Set(lvertices, 4);
                lbody->CreateFixture(&shape, 0.0f)->SetUserData(&type_floor);
            }
            {
                b2Body *rbody = m_world->CreateBody(&bd);
                rbody->SetLinearVelocity(b2Vec2(0, k_floor_velocity));
                new_floor.second = rbody;
                b2PolygonShape shape;
                const b2Vec2   rvertices[4] = {
                    b2Vec2{posX + k_down_length, 0},
                    b2Vec2{k_playfieldRightEdge, 0},
                    b2Vec2{k_playfieldRightEdge, -k_floor_thickness},
                    b2Vec2{posX + k_down_length, -k_floor_thickness},
                };
                shape.Set(rvertices, 4);
                rbody->CreateFixture(&shape, 0.0f)->SetUserData(&type_floor);
            }
        }
        floors.push_back(new_floor);
    }
    void Keyboard(unsigned char key) {
        float32 k_force = 15;
        switch(key) {
            case 'a':
                monster_handle->ApplyLinearImpulse(
                    b2Vec2(-k_force, 0), monster_handle->GetPosition(), true);
                break;
            case 'w':
                monster_handle->ApplyLinearImpulse(
                    b2Vec2(0, k_force), monster_handle->GetPosition(), true);
                break;
            case 's':
                monster_handle->ApplyLinearImpulse(
                    b2Vec2(0, -k_force), monster_handle->GetPosition(), true);
                break;
            case 'd':
                monster_handle->ApplyLinearImpulse(
                    b2Vec2(k_force, 0), monster_handle->GetPosition(), true);
                break;
            default:
                break;
        }
    }
};

#include <map>

std::map<int, int> a;

// class MyContactListener : public b2ContactListener {
//   public:
//     void BeginContact(b2Contact *contact) {}
//     void EndContact(b2Contact *contact) {}
//     void PreSolve(b2Contact *contact, const b2Manifold *oldManifold) {}
//     void PostSolve(b2Contact *contact, const b2ContactImpulse *impulse) {}
// };

#endif