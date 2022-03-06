#ifndef DEVIL_H
#define DEVIL_H

#include "../Framework/Main.h"
#include "../Framework/ParticleParameter.h"
#include "../Framework/Render.h"
#include "../Framework/Test.h"

#if defined(__APPLE__) && !defined(__IOS__)
#    include <GLUT/glut.h>
#else
#    include "GL/freeglut.h"
#endif

#include "Params.h"
#include <iostream>
#include <string>
#include <time.h>
#include <vector>
using std::string;
using std::vector;

using namespace DevilParams;
struct DevilBody {
    b2Vec2           m_offset     = b2Vec2{0, 0};
    b2Body *         m_chassis    = NULL;
    b2Body *         m_wheel      = NULL;
    b2RevoluteJoint *m_motorJoint = NULL;
    bool             m_motorOn    = false;
    float32          m_motorSpeed = 0.0f;
    b2Body *         blood        = NULL;
    int              blood_number = k_blood;
    vector<b2Body *> m_legs;
    bool             deleted = false;
};

class Devil : virtual public Test, virtual public uInfoManager {
  public:
    Devil(){};

  private:
    const float32 m_size =
        DevilParams::k_size;  // modify the size of the devil
                              // to fit the height of the floors
    void CreateBlood(DevilBody *devil_body) {
        using namespace LiquidMonsterParams;
        // body
        b2BodyDef bddef;
        bddef.type            = b2_kinematicBody;
        bddef.linearVelocity  = devil_body->m_chassis->GetLinearVelocity();
        bddef.angularVelocity = devil_body->m_chassis->GetAngularVelocity();

        float32 _angle = devil_body->m_chassis->GetAngle();
        bddef.position = devil_body->m_chassis->GetPosition() +
            2.0f * b2Vec2{cosf32(_angle + M_PI_2), sinf32(_angle + M_PI_2)};
        bddef.angle = _angle;
        // shape
        b2PolygonShape shape;
        float32        length = devil_body->blood_number / k_blood * 3.0f;
        b2Vec2 points[] = {b2Vec2{-1.5f, 0.1f}, b2Vec2{length - 1.5f, 0.1f},
                           b2Vec2{length - 1.5f, -0.1f}, b2Vec2{-1.5f, -0.1f}};
        shape.Set(points, 4);
        // fixture
        b2FixtureDef bfdef;
        bfdef.shape             = &shape;
        bfdef.filter.groupIndex = 1;

        devil_body->blood = m_world->CreateBody(&bddef);
        devil_body->blood->CreateFixture(&bfdef);
        devil_body->blood->SetUserData(info({t_devil_blood, devil_body}));
    }
    void DeleteBlood(DevilBody *devil_body) {
        if(devil_body->blood)
            m_world->DestroyBody(devil_body->blood);
    }
    void CreateLeg(DevilBody *devil_body, float32 s, const b2Vec2 &wheelAnchor,
                   float32 size) {
        b2Vec2 p1(5.4f * s * size, -6.1f * size);
        b2Vec2 p2(7.2f * s * size, -1.2f * size);
        b2Vec2 p3(4.3f * s * size, -1.9f * size);
        b2Vec2 p4(3.1f * s * size, 0.8f * size);
        b2Vec2 p5(6.0f * s * size, 1.5f * size);
        b2Vec2 p6(2.5f * s * size, 3.7f * size);

        b2FixtureDef fd1, fd2;
        fd1.filter.groupIndex = -1;
        fd2.filter.groupIndex = -1;
        fd1.density           = k_density;
        fd2.density           = k_density;

        b2PolygonShape poly1, poly2;

        if(s > 0.0f) {
            b2Vec2 vertices[3];

            vertices[0] = p1;
            vertices[1] = p2;
            vertices[2] = p3;
            poly1.Set(vertices, 3);

            vertices[0] = b2Vec2_zero;
            vertices[1] = p5 - p4;
            vertices[2] = p6 - p4;
            poly2.Set(vertices, 3);
        }
        else {
            b2Vec2 vertices[3];

            vertices[0] = p1;
            vertices[1] = p3;
            vertices[2] = p2;
            poly1.Set(vertices, 3);

            vertices[0] = b2Vec2_zero;
            vertices[1] = p6 - p4;
            vertices[2] = p5 - p4;
            poly2.Set(vertices, 3);
        }

        fd1.shape = &poly1;
        fd2.shape = &poly2;

        b2BodyDef bd1, bd2;
        bd1.type     = b2_dynamicBody;
        bd2.type     = b2_dynamicBody;
        bd1.position = devil_body->m_offset;
        bd2.position = p4 + devil_body->m_offset;

        bd1.angularDamping = 10.0f;
        bd2.angularDamping = 10.0f;

        b2Body *body1 = m_world->CreateBody(&bd1);
        b2Body *body2 = m_world->CreateBody(&bd2);

        devil_body->m_legs.push_back(body1);
        devil_body->m_legs.push_back(body2);

        body1->CreateFixture(&fd1);
        body2->CreateFixture(&fd2);

        b2DistanceJointDef djd;

        // Using a soft distance constraint can reduce some jitter.
        // It also makes the structure seem a bit more fluid by
        // acting like a suspension system.
        djd.dampingRatio = 0.5f;
        djd.frequencyHz  = 20.0f;

        djd.Initialize(body1, body2, p2 + devil_body->m_offset,
                       p5 + devil_body->m_offset);
        m_world->CreateJoint(&djd);

        djd.Initialize(body1, body2, p3 + devil_body->m_offset,
                       p4 + devil_body->m_offset);
        m_world->CreateJoint(&djd);

        djd.Initialize(body1, devil_body->m_wheel, p3 + devil_body->m_offset,
                       wheelAnchor + devil_body->m_offset);
        m_world->CreateJoint(&djd);

        djd.Initialize(body2, devil_body->m_wheel, p6 + devil_body->m_offset,
                       wheelAnchor + devil_body->m_offset);
        m_world->CreateJoint(&djd);

        b2RevoluteJointDef rjd;

        rjd.Initialize(body2, devil_body->m_chassis, p4 + devil_body->m_offset);
        m_world->CreateJoint(&rjd);

        // used in collision destruction
        using namespace LiquidMonsterParams;
        body1->SetUserData(info({t_devil, devil_body}));
        body2->SetUserData(info({t_devil, devil_body}));
    }

  protected:
    // ground is the floor on which the devil locates
    void CreateDevil(DevilBody *devil_body, b2Vec2 pos) {
        using namespace LiquidMonsterParams;
        using namespace FloorParams;
        devil_body->m_motorSpeed = 1.0f;
        devil_body->m_motorOn    = true;
        float32 x                = pos.x;
        float32 y                = pos.y;
        printf("x,y: %f %f\n", x, y);
        devil_body->m_offset.Set(x * m_size, (y)*m_size);

        b2Vec2 pivot(0, (0.8f) * m_size);

        // Chassis
        {
            b2PolygonShape shape;
            shape.SetAsBox(2.5f * m_size, 1.0f * m_size);

            b2FixtureDef sd;
            sd.density           = k_density;
            sd.shape             = &shape;
            sd.filter.groupIndex = -1;
            b2BodyDef bd;
            bd.type               = b2_dynamicBody;
            bd.position           = pivot + devil_body->m_offset;
            devil_body->m_chassis = m_world->CreateBody(&bd);
            devil_body->m_chassis->CreateFixture(&sd);
            devil_body->m_chassis->SetUserData(info({t_devil, devil_body}));
        }

        {
            b2CircleShape shape;
            shape.m_radius = 1.6f * m_size;

            b2FixtureDef sd;
            sd.density           = k_density;
            sd.shape             = &shape;
            sd.filter.groupIndex = -1;
            b2BodyDef bd;
            bd.type             = b2_dynamicBody;
            bd.position         = pivot + devil_body->m_offset;
            devil_body->m_wheel = m_world->CreateBody(&bd);
            devil_body->m_wheel->CreateFixture(&sd);
            b2RevoluteJointDef jd;
            jd.Initialize(devil_body->m_wheel, devil_body->m_chassis,
                          pivot + devil_body->m_offset);
            jd.collideConnected = false;
            jd.motorSpeed       = devil_body->m_motorSpeed;
            jd.maxMotorTorque   = 400.0f;
            jd.enableMotor      = devil_body->m_motorOn;
            devil_body->m_motorJoint =
                (b2RevoluteJoint *)m_world->CreateJoint(&jd);
            devil_body->m_wheel->SetUserData(info({t_devil, devil_body}));
        }

        b2Vec2 wheelAnchor = pivot + b2Vec2(0.0f * m_size, -0.8f * m_size);

        CreateLeg(devil_body, -1.0f, wheelAnchor, m_size);
        CreateLeg(devil_body, 1.0f, wheelAnchor, m_size);

        devil_body->m_wheel->SetTransform(devil_body->m_wheel->GetPosition(),
                                          120.0f * b2_pi / 180.0f);
        CreateLeg(devil_body, -1.0f, wheelAnchor, m_size);
        CreateLeg(devil_body, 1.0f, wheelAnchor, m_size);

        devil_body->m_wheel->SetTransform(devil_body->m_wheel->GetPosition(),
                                          -120.0f * b2_pi / 180.0f);
        CreateLeg(devil_body, -1.0f, wheelAnchor, m_size);
        CreateLeg(devil_body, 1.0f, wheelAnchor, m_size);

        CreateBlood(devil_body);
    }

    void DeleteDevil(DevilBody *devil_body) {
        printf("deleted\n");
        if(devil_body == NULL)
            return;
        if(devil_body->deleted == true)
            return;
        DeleteBlood(devil_body);
        devil_body->deleted = true;
        m_world->DestroyBody(devil_body->m_chassis);
        m_world->DestroyBody(devil_body->m_wheel);
        while(!devil_body->m_legs.empty()) {
            m_world->DestroyBody(devil_body->m_legs.back());
            devil_body->m_legs.pop_back();
        }
        if(devil_body->m_chassis != NULL)
            devil_body->m_chassis = NULL;
        if(devil_body->m_wheel != NULL)
            devil_body->m_wheel = NULL;
    }

    void UpdateBlood(DevilBody *devil_body) {
        if(devil_body == NULL) {
            return;
        }
        if(devil_body->deleted == true) {
            return;
        }
        DeleteBlood(devil_body);
        CreateBlood(devil_body);
    }
};
#endif