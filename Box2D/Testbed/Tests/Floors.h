#ifndef FLOORS_H
#define FLOORS_H

#include "../Framework/Main.h"
#include "../Framework/ParticleParameter.h"
#include "../Framework/Render.h"
#include "../Framework/Test.h"

#if defined(__APPLE__) && !defined(__IOS__)
#    include <GLUT/glut.h>
#else
#    include "GL/freeglut.h"
#endif

#include "Devil.h"
#include "Params.h"
#include <Testbed/Framework/Test.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <queue>
#include <string>
#include <vector>
//#include "LiquidMonster.h"

using std::string;
using std::vector;

struct FloorLayer {
    vector<b2Body *> p_floor;
    bool             is_devil = false;
    DevilBody        p_DevilOnFloor;
};

class BaseFloor : virtual public Test,
                  virtual public Devil,
                  virtual public uInfoManager {
  public:
    BaseFloor(){};
  protected:
    // store all pointers to the floor
    std::queue<FloorLayer *> floors;

    // create floor;
    virtual void CreateFloor(bool last) = 0;
    // delete floor
    void CreateDevil(FloorLayer *floorlayer) {
        floorlayer->is_devil = true;
        Devil::CreateDevil(&floorlayer->p_DevilOnFloor,
                            floorlayer->p_floor.front()->GetPosition() +
                                b2Vec2{0.1f, 0.1f});
    }
    void DeleteDevil(FloorLayer *floorlayer) {
        Devil::DeleteDevil(&floorlayer->p_DevilOnFloor);
    }
    // add a devil to the ground?
    virtual void AddDevil(FloorLayer *ground) {
        using namespace DevilParams;
        srand((unsigned)time(NULL));
        int is_devil = (rand() % k_devil_appearance_probability == 0 ? 1 : 0);
        if(is_devil) {
            using namespace LiquidMonsterParams;
            CreateDevil(ground);
        }
    }
    virtual void DeleteFloor() {
        auto &single_floor = floors.front();
        printf("delete floor\n");
        if(single_floor->is_devil) {
            DeleteDevil(single_floor);
        }
        for(auto &t : single_floor->p_floor) {
            m_world->DestroyBody(t);
        }
        floors.pop();
    }
};

class SoftFloor : virtual public BaseFloor {
  public:
    SoftFloor() {}
  protected:
    void CreateFloor(bool last) {
        // Create Ground
        using namespace LiquidMonsterParams;
        using namespace FloorParams;
        std::vector<FloorLayer *> single_floor;
        // create two side's "anchor"
        b2BodyDef bd;
        bd.type = b2_kinematicBody;
        float32 posX_1, posX_2;
        if(last) {
            posX_1 = k_playfieldLeftEdge;
            posX_2 = posX_1 + k_softfloor_width;
        }
        else {
            posX_2 = k_playfieldRightEdge;
            posX_1 = posX_2 - k_softfloor_width;
        }
        float32 posY = k_playfieldBottomEdge + 3;

        FloorLayer *new_layer = new FloorLayer;

        bd.position.Set(posX_1, posY);
        b2Body *left_ground = m_world->CreateBody(&bd);
        left_ground->SetLinearVelocity(b2Vec2(0, k_floor_speed));

        bd.position.Set(posX_2, posY);
        b2Body *right_ground = m_world->CreateBody(&bd);
        right_ground->SetLinearVelocity(b2Vec2(0, k_floor_speed));

        b2CircleShape side_shape;
        side_shape.m_radius = 0.5f;

        b2FixtureDef fixture;
        fixture.shape = &side_shape;
        // fixture.filter.groupIndex = ~(1 << 5);

        left_ground->CreateFixture(&fixture);
        left_ground->SetUserData(info({t_softfloor, NULL}));

        right_ground->CreateFixture(&fixture);
        right_ground->SetUserData(info({t_softfloor, NULL}));

        // left = left ^ 1;
        // create a bridge
        const static int32 N = 20;
        // const static float32 k_softfloor_length = 20.0f;
        float32        k_single_length = (posX_2 - posX_1) / N;
        float32        k_floor_height  = 0.25f;
        b2PolygonShape shape;

        shape.SetAsBox(k_single_length / 1.7, k_floor_height);

        b2FixtureDef fd;
        fd.shape             = &shape;
        fd.density           = 1.0f;
        fd.friction          = 1.0f;
        fd.filter.groupIndex = ~(1 << 5);
        fixture.restitution  = 0.1;

        b2RevoluteJointDef jd;
        new_layer->p_floor.push_back(left_ground);
        b2Body *prevGround = left_ground;
        for(int32 i = 0; i < N; ++i) {
            b2BodyDef bd;
            bd.type = b2_dynamicBody;
            bd.position.Set(posX_1 + k_single_length * (i + 0.5), posY);
            b2Body *thisGround = m_world->CreateBody(&bd);
            new_layer->p_floor.push_back(thisGround);
            thisGround->CreateFixture(&fd);
            thisGround->SetUserData(info({t_softfloor, NULL}));
            b2Vec2 anchor(posX_1 + k_single_length * i, posY);
            jd.Initialize(prevGround, thisGround, anchor);
            jd.enableLimit = true;
            jd.lowerAngle  = -M_PI / 300;
            jd.upperAngle  = +M_PI / 300;
            m_world->CreateJoint(&jd);

            prevGround = thisGround;
        }
        new_layer->p_floor.push_back(right_ground);

        b2Vec2 anchor(posX_1 + k_single_length * N, k_playfieldBottomEdge);
        jd.Initialize(prevGround, right_ground, anchor);
        m_world->CreateJoint(&jd);

        // create a devil on the floor
        CreateDevil(new_layer);

        floors.push(new_layer);
    }
};

class HardFloor : virtual public BaseFloor {
    /*
    ------------------------------------	//TOP EDGE
    |\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/|	//THORNS
    |                                  |
    |___________________________       |5	-> phead_floor
    |                                  |
    |       ___________________________|4
    |                                  |	/
    |___________________________       |3	|  floors' moving direction
    |                                  |	|
    |       ___________________________|2
    |                                  |
    |___________________________       |1(st layer)
    |                                  |
    |       ___________________________|0	-> ptail_floor
    ------------------------------------	//BOTTOM EDGE
    */
   public:
    HardFloor() {}
  protected:
    void CreateFloor(bool last) {
        // Create Ground
        using namespace LiquidMonsterParams;
        using namespace FloorParams;
        b2BodyDef bd;
        bd.type = b2_kinematicBody;

        FloorLayer *new_layer = new FloorLayer;
        float32     posX_1, posX_2;
        if(last) {
            posX_1 = k_playfieldLeftEdge;
            posX_2 = posX_1 + k_hardfloor_width;
        }
        else {
            posX_2 = k_playfieldRightEdge;
            posX_1 = posX_2 - k_hardfloor_width;
        }
        float32 posY = k_playfieldBottomEdge;
        bd.position.Set(0, posY);

        b2Body *ground = m_world->CreateBody(&bd);
        ground->SetLinearVelocity(b2Vec2(0, k_floor_speed));
        ground->SetUserData(info({t_hardfloor, NULL}));
        b2PolygonShape shape;
        const float32  k_hardfloor_thickness = 1.0f;
        const b2Vec2   vertices[4]           = {
            b2Vec2(posX_1, 0), b2Vec2(posX_1, -k_hardfloor_thickness),
            b2Vec2(posX_2, 0), b2Vec2(posX_2, -k_hardfloor_thickness)};
        shape.Set(vertices, 4);

        b2FixtureDef fixture;
        fixture.shape       = &shape;
        fixture.restitution = 0.1;
        // fixture.filter.groupIndex = ~(1 << 5);
        ground->CreateFixture(&fixture);

        new_layer->p_floor.push_back(ground);
        AddDevil(new_layer);

        floors.push(new_layer);
    }
};

class Floor : virtual public HardFloor, virtual public SoftFloor {
  public:
    /*
    ------------------------------------	// TOP EDGE
    |\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/|	// THORNS
    |                                  |
    |___________________________       |5	-> phead_floor
    |                                  |
    |       ___________________________|4
    |                                  |	/
    |___________________________       |3	|  floors' moving direction
    |                                  |	|
    |       ___________________________|2
    |                                  |
    |___________________________       |1(st layer)
    |                                  |
    |       ___________________________|0	-> ptail_floor
    ------------------------------------	// BOTTOM EDGE
    */
    Floor() {}
  protected:
    void CreateFloor(bool __last = 0) final {
        static bool last = 0;
        // ...
        last = !last;
        if(rand() & 1) {
            HardFloor::CreateFloor(last);
        }
        else {
            SoftFloor::CreateFloor(last);
        }
    }
};
#endif