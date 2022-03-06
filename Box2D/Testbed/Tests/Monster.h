#ifndef MONSTER_H
#define MONSTER_H
#include "Params.h"
#include <algorithm>
#include <deque>
#include <set>
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

class Monster : virtual public Test, virtual public uInfoManager {
  public:
    Monster() {}

  protected:
    int              particle_lowerbound;
    b2Body *         monster_handle;
    b2ParticleGroup *monster_particles;
    void             create_monster() {
        using namespace LiquidMonsterParams;
        b2Vec2 begin_position = {0, 10};
        m_particleSystem2->SetDensity(0.5);
        m_particleSystem2->SetRadius(0.25f);
        m_particleSystem2->SetGravityScale(0.4);
        {
            // add a little ball as a handle
            b2BodyDef bd;
            bd.type = b2_dynamicBody;
            // bd.userData         = &type_ball;
            b2CircleShape shape = b2CircleShape();
            shape.m_radius      = k_handle_radius;
            bd.gravityScale     = 0.5;
            b2FixtureDef bf;
            bf.shape       = &shape;
            bf.density     = 2;
            bf.restitution = 1;
            bd.position    = begin_position;
            b2Body *body   = m_world->CreateBody(&bd);
            body->CreateFixture(&bf);
            body->SetUserData(info({t_ball, NULL}));
            monster_handle = body;
        }
        // a chain around liquid monster
        m_particleSystem->SetDensity(0.5);
        m_particleSystem->SetRadius(0.15f);
        m_particleSystem->SetGravityScale(0.2);
        {
            b2CircleShape shape;
            shape.m_p      = begin_position;
            shape.m_radius = 3.0f;

            b2ParticleGroupDef pd;
            // pd.userData   = &type_ball;
            pd.flags = b2_springParticle | b2_barrierParticle |
                b2_fixtureContactFilterParticle;
            pd.groupFlags = b2_solidParticleGroup;
            pd.shape      = &shape;
            pd.color.Set(0xff, 0x00, 0x00, 255);
            m_particleSystem->CreateParticleGroup(pd)->SetUserData(
                info({t_ball, NULL}));
        }
        b2CircleShape innershape;
        innershape.m_p.Set(0, 0);
        innershape.m_radius = 1.5f;
        b2Transform trans   = b2Transform();
        trans.Set(begin_position, 0);
        m_particleSystem->DestroyParticlesInShape(innershape, trans);

        // liquid monster
        {
            b2CircleShape shape;
            shape.m_p      = begin_position;
            shape.m_radius = 1.5f;
            b2ParticleGroupDef pd;
            pd.flags = b2_viscousParticle | b2_fixtureContactFilterParticle;
            // pd.strength = 0.05f;
            // pd.groupFlags = 0;
            pd.shape = &shape;
            pd.color.Set(0x66, 0xcc, 0xff, 255);
            auto it = m_particleSystem->CreateParticleGroup(pd);
            monster_particles   = it;
            particle_lowerbound = it->GetParticleCount() / 3;
        }
    }

    void add_monster_particle() {
        const int ADDN = 5;
        using namespace LiquidMonsterParams;
        for(int i = 1; i <= ADDN; i++) {
            b2ParticleDef pdf;
            pdf.color.Set(0x66, 0xcc, 0xff, 255);
            pdf.position = monster_handle->GetPosition() +
                b2Vec2{0, -1.1 * k_handle_radius};
            pdf.flags = b2_viscousParticle | b2_fixtureContactFilterParticle;
            pdf.group = monster_particles;
            m_particleSystem->CreateParticle(pdf);
        }
    }

    void shoot_a_ball() {
        // find near by particles and delete them to shoot a ball outside the
        // monster how to solve it in a single particle system? after cross the
        // red border, this will be a right one !
        using namespace LiquidMonsterParams;
        static const int N = 10;
        if(monster_particles->GetParticleCount() < N) {
            return;
        }
        delete_near_N(N);
        {
            // create a ball
            b2CircleShape shape;
            b2Vec2        handle_velo = monster_handle->GetLinearVelocity() -
                b2Vec2{0, k_floor_velocity};
            handle_velo = handle_velo;
            shape.m_p.Set(
                monster_handle->GetPosition().x +
                    4 * k_handle_radius * handle_velo.x / handle_velo.Length(),
                monster_handle->GetPosition().y +
                    4 * k_handle_radius * handle_velo.y / handle_velo.Length());
            shape.m_radius = 0.5f;
            b2ParticleGroupDef pd;
            pd.userData = info({t_ball, NULL});
            pd.flags    = b2_elasticParticle | b2_fixtureContactFilterParticle;
            pd.shape    = &shape;
            pd.linearVelocity = handle_velo * 3 + b2Vec2{0, k_floor_velocity};
            pd.lifetime       = life_period;
            pd.color.Set(0x00, 0xff, 0x00, 255);
            auto it = m_particleSystem2->CreateParticleGroup(pd);
            it->SetUserData(info({t_ball, NULL}));
        }
    }
    b2Mat22 getRotation(float32 angle) {
        return b2Mat22{cosf(angle), -sinf(angle), sinf(angle), cosf(angle)};
    }
    void shoot_separate() {
        using namespace LiquidMonsterParams;
        static const int N = 10;
        static const int M = 3;
        if(monster_particles->GetParticleCount() < N) {
            return;
        }
        delete_near_N(N);
        {
            // create a "tiannv sanhua"
            b2Vec2 handle_velo = monster_handle->GetLinearVelocity() -
                b2Vec2{0, k_floor_velocity};
            ;
            b2ParticleGroupDef g_def;
            g_def.lifetime = life_period;
            auto g_ref     = m_particleSystem2->CreateParticleGroup(g_def);
            for(int r = 1; r <= 2; r++) {
                float32 R = 3 * r * k_handle_radius;
                for(int i = 0; i < M; i++) {
                    // - Pi/6 to Pi/6
                    float32 angle = -range_angle + i * range_angle * 2 / M;
                    b2Vec2  pos   = b2Mul(getRotation(angle),
                                       R * handle_velo / handle_velo.Length());
                    b2ParticleDef p_def;
                    p_def.color.Set(0x00, 0xff, 0x00, 255);
                    p_def.position = monster_handle->GetPosition() + pos;
                    p_def.flags =
                        b2_elasticParticle | b2_fixtureContactFilterParticle;
                    p_def.group    = g_ref;
                    p_def.lifetime = life_period;
                    p_def.velocity = 2 * pos / R *
                            monster_handle->GetLinearVelocity().Length() +
                        b2Vec2{0, k_floor_velocity};
                    m_particleSystem2->CreateParticle(p_def);
                }
            }
        }
    }

  private:
    void delete_near_N(const int &N) {
        std::set<std::pair<float32, int>> S;
        {
            int32   offset          = monster_particles->GetBufferIndex();
            b2Vec2  handle_position = monster_handle->GetPosition();
            int32   m_size          = monster_particles->GetParticleCount();
            b2Vec2 *pos =
                monster_particles->GetParticleSystem()->GetPositionBuffer();
            for(int32 i = 0; i < m_size; i++) {
                int x = i + offset;
                S.insert(
                    std::make_pair(b2Distance(handle_position, pos[x]), x));
            }
            int32 delete_cnt = 0;
            for(auto &t : S) {
                monster_particles->GetParticleSystem()->DestroyParticle(
                    t.second);
                printf("%d\n", t.second);
                if(++delete_cnt >= N)
                    break;
            }
        }
    }
};

#endif