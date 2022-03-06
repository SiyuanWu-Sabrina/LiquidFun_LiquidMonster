#ifndef LIQUID_MONSTER_H
#define LIQUID_MONSTER_H
#include "Box.h"
#include "Devil.h"
#include "Floors.h"
#include "Monster.h"
#include "Params.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
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

// The following parameters are not static const members of the LiquidMonster
// class with values assigned inline as it can result in link errors when using
// gcc.

class myContactFilter : public b2ContactFilter {
  public:
    bool ShouldCollide(b2Fixture *fixtureA, b2Fixture *fixtureB) {
        if((fixtureA->GetFilterData().groupIndex == -1 &&
            fixtureB->GetFilterData().groupIndex == -1) ||
           fixtureA->GetFilterData().groupIndex == 1 ||
           fixtureB->GetFilterData().groupIndex == 1) {
            return false;
        }
        else {
            return fixtureA->GetFilterData().maskBits &
                fixtureB->GetFilterData().maskBits;
        }
    }
    bool ShouldCollide(b2Fixture *fixture, b2ParticleSystem *particleSystem,
                       int32 particleIndex) {
        if(fixture->GetFilterData().groupIndex == 1) {
            // printf("%d %d\n", fixture->GetFilterData().groupIndex,
            //    particleIndex);
            return false;
        }
        else {
            return true;
        }
    }
};

inline float32 operator*(const b2Vec2 &v1, const b2Vec2 &v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

class LiquidMonster : virtual public Monster,
                      virtual public Floor,
                      virtual public Box {
  public:
    LiquidMonster() {
        using namespace LiquidMonsterParams;
        srand(time(NULL));
        filter = new myContactFilter;
        m_world->SetContactFilter(filter);
        m_world->SetGravity({0.0, -10.0});
        score = 0;

        // ----------
        this->create_box();
        this->create_monster();
    }
    ~LiquidMonster() {
        delete filter;
    }
    static Test *Create() {
        return new LiquidMonster;
    }
    void Step(Settings *settings) {
        using namespace LiquidMonsterParams;

        if(isLose()) {
            m_debugDraw.DrawString(5, m_textLine, "you lose! Final score: %.2f",
                                   float32(score));
            m_textLine += DRAW_STRING_NEW_LINE;
            settings->pause = 1;
        }
        Test::Step(settings);
        if(isLose()) {
            return;
        }
        // set stepping
        static int i = 0;
        i++;
        if(!settings->pause)
            score += k_time_score * monster_particles->GetParticleCount() / particle_lowerbound;
        static const int k_rounds_add_particle = 100;
        if(i % k_rounds_add_particle == 0)
            add_monster_particle();
        // std::printf("step:%d\n", i);
        Update();

        // Update devil's blood
        int update_period = 3;
        if(i % update_period == 0) {
            auto q = floors;
            while(!q.empty()) {
                UpdateBlood(&q.front()->p_DevilOnFloor);
                q.pop();
            }
        }
        // monster particles and thorn body
        using namespace DevilParams;
        for(int i = 0; i < m_particleSystem->GetBodyContactCount(); i++) {
            auto it = m_particleSystem->GetBodyContacts()[i];
            if(get_type(it.body->GetUserData()) == t_thorn)
                m_particleSystem->DestroyParticle(it.index);
        }
        // ball particles and devil body
        std::set<int> used_particle;
        for(int i = 0; i < m_particleSystem2->GetBodyContactCount(); i++) {
            auto it = m_particleSystem2->GetBodyContacts()[i];
            // printf("%f\n",it.mass);
            if(get_type(it.body->GetUserData()) == t_devil) {
                auto *t = get_pointer<DevilBody>(it.body->GetUserData());
                // printf("%d\n",t->m_points);
                if(t->deleted == true || used_particle.count(it.index))
                    continue;
                t->blood_number -= 1;
                if(t->blood_number <= 0) {
                    Devil::DeleteDevil(t);
                    score += k_devil_score;
                }
                used_particle.insert(it.index);
                m_particleSystem2->DestroyParticle(it.index);
            }
        }
        // delete downward particles
        {
            b2PolygonShape shape;
            // float32 k_mid = (k_playfieldLeftEdge + k_playfieldRightEdge) / 2;
            const b2Vec2 vertices[4] = {
                b2Vec2(k_playfieldLeftEdge - k_wallThickness - 50,
                       -k_ground_height * 2),

                b2Vec2(k_playfieldRightEdge + k_wallThickness + 50,
                       -k_ground_height * 2),
                b2Vec2(k_playfieldRightEdge + k_wallThickness + 50,
                       -k_groundThickness - k_ground_height * 2),
                b2Vec2(k_playfieldLeftEdge - k_wallThickness - 50,
                       -k_groundThickness - k_ground_height * 2)

            };
            shape.Set(vertices, 4);
            b2Transform trans;
            trans.SetIdentity();
            m_particleSystem->DestroyParticlesInShape(shape, trans);
            m_particleSystem2->DestroyParticlesInShape(shape, trans);
        }

        Draw();
    }
    void Keyboard(unsigned char key) {
        float32 k_force = 15;
        using namespace LiquidMonsterParams;
        using std::max;
        b2Vec2 v =
            monster_handle->GetLinearVelocity() - b2Vec2{0, k_floor_velocity};

        switch(key) {
            case 'a':
                monster_handle->ApplyLinearImpulse(
                    b2Vec2{-k_force /
                               (log2(log10(max(-v.x, 0.0f) + 1) + 1) + 1),
                           0},
                    monster_handle->GetPosition(), true);
                break;
            case 'w':
                monster_handle->ApplyLinearImpulse(
                    b2Vec2{0,
                           k_force / (log2(log10(max(v.y, 0.0f) + 1) + 1) + 1)},
                    monster_handle->GetPosition(), true);
                break;
            case 's':
                monster_handle->ApplyLinearImpulse(
                    b2Vec2{0,
                           -k_force /
                               (log2(log10(max(-v.y, 0.0f) + 1) + 1) + 1)},
                    monster_handle->GetPosition(), true);
                break;
            case 'd':
                monster_handle->ApplyLinearImpulse(
                    b2Vec2{k_force / (log2(log10(max(v.x, 0.0f) + 1) + 1) + 1),
                           0},
                    monster_handle->GetPosition(), true);
                break;
            case 'q':
                shoot_a_ball();
                break;
            case 'e':
                shoot_separate();
            default:
                break;
        }
    }

  private:
    myContactFilter *filter;
    float32          score;
    // record floors
    bool isLose() {
        return monster_particles->GetParticleCount() < particle_lowerbound;
    }
    void Update() {
        // create and delete floors;
        using namespace LiquidMonsterParams;
        using namespace FloorParams;
        if(floors.empty() ||
           floors.back()->p_floor.front()->GetPositionY() >
               k_playfieldBottomEdge + k_layerHeight) {
            CreateFloor();
        }
        while(!floors.empty() &&
              floors.front()->p_floor.front()->GetPositionY() >
                  k_playfieldTopEdge - k_thornHeight) {
            DeleteFloor();
        }
    }
    void Draw() {
        {
            char s_score[100];
            sprintf(s_score, "now particles: %d",
                    int(monster_particles->GetParticleCount()));
            m_debugDraw.DrawString(5, m_textLine, s_score);
            m_textLine += DRAW_STRING_NEW_LINE;
        }
        {
            char s_score[100];
            sprintf(s_score, "now score: %.2f", float32(score));
            m_debugDraw.DrawString(5, m_textLine, s_score);
            m_textLine += DRAW_STRING_NEW_LINE;
        }
    }
};

#endif