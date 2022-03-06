#ifndef Tanks_H
#define Tanks_H
#define MAX_PLAYERS 4
#define PLAYERS 4
#define MIN_VEL 0
#define MAX_VEL 100
#define START_FUEL 500
#define MAX_HEIGHT 7
#define MIN_HEIGHT 3
#include <algorithm>
#include <iostream>

class PVFX //Particle Visual Effects, Used solely to create explosions. Courtesy of Sparky.h
{
public:
	PVFX(b2ParticleSystem *particleSystem, const b2Vec2 &origin,
				float32 size, float32 speed, float32 lifetime,
				uint32 particleFlags)
	{
		b2CircleShape shape; //houses particles
		shape.m_p = origin;
		shape.m_radius = size;

		b2ParticleGroupDef pd;
		pd.flags = particleFlags;
		pd.shape = &shape;
		m_origColor.Set(rand() % 256, rand() % 256, rand() % 256, 255);
		pd.color = m_origColor;
	
		m_particleSystem = particleSystem;

		m_pg = m_particleSystem->CreateParticleGroup(pd);

		m_initialLifetime = m_remainingLifetime = lifetime;
		m_halfLifetime = m_initialLifetime * 0.5f;

		int32 bufferIndex = m_pg->GetBufferIndex();
		b2Vec2 *pos = m_particleSystem->GetPositionBuffer();
		b2Vec2 *vel = m_particleSystem->GetVelocityBuffer();
		for (int i = bufferIndex; i < bufferIndex + m_pg->GetParticleCount(); i++)
		{
			vel[i] = pos[i] - origin;
			vel[i] *= speed;
		}
	}

	~PVFX()
	{
	  m_pg->DestroyParticles(false);
	}

	float32 ColorCoeff()
	{
		if (m_remainingLifetime >= m_halfLifetime)
		{
			return 1.0f;
		}
		return 1.0f - ((m_halfLifetime - m_remainingLifetime) /
					   m_halfLifetime);
	}

	void Step(float32 dt)
	{
		if (m_remainingLifetime > 0.0f)
		{
          m_remainingLifetime = std::max(m_remainingLifetime - dt, 0.0f);
			float32 coeff = ColorCoeff();

			b2ParticleColor *colors = m_particleSystem->GetColorBuffer();
			int bufferIndex = m_pg->GetBufferIndex();

			for (int i = bufferIndex;
				 i < bufferIndex + m_pg->GetParticleCount(); i++)
			{
				b2ParticleColor &c = colors[i];
				c *= coeff;
				c.a = m_origColor.a;
			}
		}
	}
	bool IsDone() { return m_remainingLifetime <= 0.0f; }

private:
	float32 m_initialLifetime;
	float32 m_remainingLifetime;
	float32 m_halfLifetime;
	b2ParticleGroup *m_pg;
	b2ParticleSystem *m_particleSystem;
	b2ParticleColor m_origColor;
};

struct Player // stores data for players so it can be easily toggled for multiple players
{
	b2Body* m_tank;

	b2Fixture* m_turret;

	b2Body* m_wheel1;
	b2Body* m_wheel2;
    b2Body* m_wheel3;
	b2Body* m_wheel4;
	b2Body* m_wheel5;

	b2Body* m_cannon;
	
	b2WheelJoint* m_spring1;
	b2WheelJoint* m_spring2;
    b2WheelJoint* m_spring3;
	b2WheelJoint* m_spring4;
	b2WheelJoint* m_spring5;

	b2RevoluteJoint* m_rotate;

}play[MAX_PLAYERS];

class Tanks : public Test
{
public:
	Tanks()
	{
		turn = 0; // which player's turn
		ang = 0.0f * b2_pi; // direction of cannon
		m_speed = 12.0f; //tank motor speed
		m_rspeed = 0.85f * b2_pi; //cannon swivel speed
		leftBound = -40.0f; //left stage boundary
		rightBound = 40.0f; //right stage boundary
		numPlayers = 0; //total number of players
		remPlayers = 0; //remaining number of players
		len = (rightBound - leftBound) / 0.5625; //amount of columns to span the stage
		gameOver = false; //indicates whether the game has ended
		startPos[0] = leftBound;
        m_world->SetGravity(b2Vec2{0,-10});
		for (i = 1; i < MAX_PLAYERS + 1; i++)
		{
			startPos[i] = startPos[i - 1] + ((rightBound - leftBound) / (MAX_PLAYERS + 1));
		}
		
		m_VFXIndex = 0; //necessary for particle explosions
		for (i = 0; i < c_maxVFX; i++) //memset
		{
			m_VFX[i] = NULL;
		}
		m_particleSystem->SetRadius(0.25f);

		generateTerrain(false); // true if you want superflat mode
		
		for (i = 0; i < PLAYERS; i ++)	newPlayer(startPos[i + 1], 0.0f, i);
		
		// ded[2] = true;
		// remPlayers = 3;
		m_bullet = NULL;

		TestMain::SetRestartOnParticleParameterChange(false);
		TestMain::SetParticleParameterValue(b2_powderParticle);
	}

	void generateTerrain(bool superFlat = true)
	{
		{//boundaries
			b2BodyDef bd;
			b2Body* ground = m_world->CreateBody(&bd);

			b2EdgeShape shape;

			b2FixtureDef fd;
			fd.shape = &shape;
			fd.density = 0.0f;
			fd.friction = 1.2f;

			shape.Set(b2Vec2(leftBound, -10.0f), b2Vec2(rightBound, -10.0f));
			ground->CreateFixture(&fd);

            shape.Set(b2Vec2(leftBound, -10.0f), b2Vec2(leftBound, 0.0f));
			ground->CreateFixture(&fd);

            shape.Set(b2Vec2(rightBound, -10.0f), b2Vec2(rightBound, 0.0f));
			ground->CreateFixture(&fd);
		}

        {//initiate ground blocks
			float32 a = 0.25f;
			b2PolygonShape shape;
			shape.SetAsBox(a, a);
            b2Vec2 pos(leftBound, -9.0f);
            b2Vec2 deltaX(0.5625f, 0.0f); //gives space for the "skin"
			b2Vec2 deltaY(0.0f, 0.5625f);

			for (int i = 0; i < len; i ++)
			{
                pos += deltaX;
                for (int j = 0; j < MIN_HEIGHT; j ++)
                {
                    b2BodyDef bd;
                    bd.type = b2_dynamicBody;
                    bd.position = pos;
                    b2Body* body = m_world->CreateBody(&bd);
                    body->CreateFixture(&shape, 5.0f);
                    pos += deltaY;
                }
                pos -= deltaY * MIN_HEIGHT;
			}
		}
		if(!superFlat) //generates semi-random terrain
		{
			int tnk = 1; //spawning points are flat-ish
			float32 posit = leftBound; //stops oscillating when posit approaches
			int osc = rand() % 10; //oscillation
			int cur = (rand() % (MAX_HEIGHT - MIN_HEIGHT)) + 1; //current layers
			float32 a = 0.25f;
			b2PolygonShape shape;
			shape.SetAsBox(a, a);
			b2Vec2 deltaX(0.5625f, 0.0f);
			b2Vec2 deltaY(0.0f, 0.5625f);
            b2Vec2 pos = b2Vec2(leftBound, -9.0f) + (MIN_HEIGHT * deltaY);
           
			
			for (int i = 0; i < len; i ++)
			{
                pos += deltaX;
                for (int j = 0; j < cur; j ++)
                {
                    b2BodyDef bd;
                    bd.type = b2_dynamicBody;
                    bd.position = pos;
                    b2Body* body = m_world->CreateBody(&bd);
                    body->CreateFixture(&shape, 5.0f);
                    pos += deltaY;
                }
                pos -= deltaY * cur;
				if((osc == 0 && cur != 0) && b2Abs(posit - startPos[tnk]) > 2.0f) cur --; //WHOA HOW DID THAT WORK?
				else if((osc == 1 && cur != MAX_HEIGHT - MIN_HEIGHT) && b2Abs(posit - startPos[tnk]) > 2.0f) cur ++;
				if (posit - startPos[tnk] >= 2.0) tnk ++;
				osc = rand() % 5;

				posit += 0.5625f;
			}
		}
		return;
	}

	void newPlayer(float32 x, float32 y, int ID)
	{
		b2PolygonShape chassis; //creates shapes
		b2Vec2 vertices[12];
		vertices[0].Set(-1.5f, -0.3f);
		vertices[1].Set(-0.9f, -0.9f);
		vertices[2].Set(0.9f, -0.9f);
		vertices[3].Set(1.5f, -0.3f);
		vertices[4].Set(1.5f, 0.3f);
		vertices[5].Set(-1.5f, 0.3f);
		chassis.Set(vertices, 6);

		b2PolygonShape cockpit;
		vertices[0].Set(-0.7f, 0.3f);
		vertices[1].Set(-0.7f, 0.9f);
		vertices[2].Set(0.7f, 0.9f);
		vertices[3].Set(0.7f, 0.3f);
		cockpit.Set(vertices, 4);

		b2PolygonShape cannon;
		vertices[0].Set(0.0f, 0.2f);
		vertices[1].Set(0.0f, -0.2f);
		vertices[2].Set(1.3f, -0.2f);
		vertices[3].Set(1.3f, 0.2f);
		cannon.Set(vertices, 4);

		b2CircleShape circle;
		circle.m_radius = 0.37f;

		b2BodyDef bd; //creates bodies and stitches together
		bd.type = b2_dynamicBody;
		bd.position.Set(0.0f + x, 1.0f + y);
		play[ID].m_tank = m_world->CreateBody(&bd);
		play[ID].m_tank->CreateFixture(&chassis, 2.0f);
		play[ID].m_turret = play[ID].m_tank -> CreateFixture(&cockpit, 4.0f);

		bd.position.Set(0.0 + x, 1.6f + y);
		play[ID].m_cannon = m_world->CreateBody(&bd);
		play[ID].m_cannon -> CreateFixture(&cannon, 1.0f);

		b2FixtureDef fd;
		fd.shape = &circle;
		fd.density = 3.0f;
		fd.friction = 15.0f;

		bd.position.Set(-1.5f + x, 0.85f + y);
		play[ID].m_wheel1 = m_world->CreateBody(&bd);
		play[ID].m_wheel1->CreateFixture(&fd);

		bd.position.Set(-0.9f + x, 0.1f + y);
		play[ID].m_wheel2 = m_world->CreateBody(&bd);
		play[ID].m_wheel2->CreateFixture(&fd);

		bd.position.Set(0.9f + x, 0.1f + y);
		play[ID].m_wheel3 = m_world->CreateBody(&bd);
		play[ID].m_wheel3->CreateFixture(&fd);

		bd.position.Set(1.5f + x, 0.85f + y);
		play[ID].m_wheel4 = m_world->CreateBody(&bd);
		play[ID].m_wheel4->CreateFixture(&fd);

		bd.position.Set(0.0f + x, 0.11f + y);
		play[ID].m_wheel5 = m_world->CreateBody(&bd);
		play[ID].m_wheel5->CreateFixture(&fd);
		
		
		b2WheelJointDef jd; //creates joints
		b2Vec2 axis(0.0f, 1.0f);
		jd.motorSpeed = 0.0f;
		jd.maxMotorTorque = 100.0f;
		jd.enableMotor = true;
		jd.frequencyHz = 5.0f;
		jd.dampingRatio = 4.0f;

		jd.Initialize(play[ID].m_tank, play[ID].m_wheel1, play[ID].m_wheel1->GetPosition(), axis);
		play[ID].m_spring1 = (b2WheelJoint*)m_world->CreateJoint(&jd);

		jd.Initialize(play[ID].m_tank, play[ID].m_wheel2, play[ID].m_wheel2->GetPosition(), axis);
		play[ID].m_spring2 = (b2WheelJoint*)m_world->CreateJoint(&jd);

		jd.Initialize(play[ID].m_tank, play[ID].m_wheel3, play[ID].m_wheel3->GetPosition(), axis);
		play[ID].m_spring3 = (b2WheelJoint*)m_world->CreateJoint(&jd);

		jd.Initialize(play[ID].m_tank, play[ID].m_wheel4, play[ID].m_wheel4->GetPosition(), axis);
		play[ID].m_spring4 = (b2WheelJoint*)m_world->CreateJoint(&jd);

		jd.Initialize(play[ID].m_tank, play[ID].m_wheel5, play[ID].m_wheel5->GetPosition(), axis);
		play[ID].m_spring5 = (b2WheelJoint*)m_world->CreateJoint(&jd);

		b2RevoluteJointDef rjd;
		rjd.Initialize(play[ID].m_tank, play[ID].m_cannon, b2Vec2(0.0f + x, 1.6f + y));
		rjd.motorSpeed = 0.0f * b2_pi;
		rjd.maxMotorTorque = 10.0f;
		rjd.enableMotor = true;
		rjd.lowerAngle = 0.05f * b2_pi;
		rjd.upperAngle = 0.95f * b2_pi;
		rjd.enableLimit = true;
		rjd.collideConnected = false;
		play[ID].m_rotate = (b2RevoluteJoint*)m_world->CreateJoint(&rjd);


		ded[ID] = false; //has been defeated
		defeat[ID] = false; //queue player up to be destroyed
		vel[ID] = (MAX_VEL + MIN_VEL) / 2; //starting power
		fuel[ID] = START_FUEL; //starting fuel
		numPlayers++;
		remPlayers++;
	
		return;
	}

	void destroy(int ID) //destroys chosen player
	{	
		if(ded[ID]) return; //already ded
		ded[ID] = true;

		play[ID].m_tank -> DestroyFixture(play[ID].m_turret); //splits cockpit from the main body
		play[ID].m_turret = NULL;
		
		b2PolygonShape cockpit;
		b2Vec2 vertices[12];
		vertices[0].Set(-0.7f, 0.3f);
		vertices[1].Set(-0.7f, 0.9f);
		vertices[2].Set(0.7f, 0.9f);
		vertices[3].Set(0.7f, 0.3f);
		cockpit.Set(vertices, 4);

		b2BodyDef bd;
		bd.type = b2_dynamicBody;
		bd.position = play[ID].m_tank -> GetPosition();
		bd.angle = play[ID].m_tank -> GetAngle();

		b2Body* brokenTurret = m_world -> CreateBody(&bd);
		play[ID].m_turret = brokenTurret -> CreateFixture(&cockpit, 1.0f);

		m_world -> DestroyJoint(play[ID].m_spring1); //destroys all joints
		play[ID].m_spring1 = NULL;
		m_world -> DestroyJoint(play[ID].m_spring2);
		play[ID].m_spring2 = NULL;
		m_world -> DestroyJoint(play[ID].m_spring3);
		play[ID].m_spring3 = NULL;
		m_world -> DestroyJoint(play[ID].m_spring4);
		play[ID].m_spring4 = NULL;
		m_world -> DestroyJoint(play[ID].m_spring5);
		play[ID].m_spring5 = NULL;
		m_world -> DestroyJoint(play[ID].m_rotate);
		play[ID].m_rotate = NULL;

		const uint32 particleFlags = TestMain::GetParticleParameterValue(); //blows up using a particle explosion
		m_contactPoint = play[ID].m_tank -> GetPosition();
		AddVFX(m_contactPoint, particleFlags);

		
		remPlayers--; 
		if(remPlayers == 1) //determines victor
		{
			for(i = 0; i < numPlayers; i ++)
			{
				if (!ded[i])
				{
					turn = i;
					break;
				}
			}
			gameOver = true;
		}
		return;
	}

	void AddVFX(const b2Vec2 &p, uint32 particleFlags) //ekusu-puroshunn
	{
		PVFX *vfx = m_VFX[m_VFXIndex];
		if (vfx != NULL)
		{
			delete vfx;
			m_VFX[m_VFXIndex] = NULL;
		}
		m_VFX[m_VFXIndex] = new PVFX(
			m_particleSystem, p, RandomFloat(1.0f, 2.0f), RandomFloat(10.0f, 20.0f),
			RandomFloat(0.5f, 1.0), particleFlags);
		if (++m_VFXIndex >= c_maxVFX)
		{
			m_VFXIndex = 0;
		}
	}

	void stopMotors() 
	{
		play[turn].m_spring1 -> SetMotorSpeed(0);
		play[turn].m_spring2 -> SetMotorSpeed(0);
		play[turn].m_spring3 -> SetMotorSpeed(0);
		play[turn].m_spring4 -> SetMotorSpeed(0);
		play[turn].m_spring5 -> SetMotorSpeed(0);
		play[turn].m_rotate -> SetMotorSpeed(0);
	}

	void nextTurn()
	{
		if(!ded[turn]) stopMotors();
		do
		{
			if(turn != numPlayers - 1)turn ++;
			else turn = 0;
		}while(ded[turn]/* && defeat[turn]*/); //goes to next player that is not yet ded/flagged to die
	}
	virtual void BeginContact(b2Contact* contact)
	{		
		Test::BeginContact(contact);
		if ((contact->GetFixtureA()-> GetUserData() != NULL ||
			contact->GetFixtureB()-> GetUserData() != NULL) && stepCount > 4) //4 is to not accidentally blow up on launch
		{
			b2WorldManifold worldManifold;
			contact->GetWorldManifold(&worldManifold);
			m_contactPoint = worldManifold.points[0];
			m_contact = true;
			contact->GetFixtureA()-> SetUserData(NULL); //object remains, no explosions
			contact->GetFixtureB()-> SetUserData(NULL);
			m_bullet = NULL;
			flag = false; //hit player
			for (i = 0; i < 4; i ++) //checks if it hit any tanks
			{
				if(contact -> GetFixtureA() -> GetBody() == play[i].m_tank ||
					contact -> GetFixtureA() -> GetBody() == play[i].m_cannon ||
					contact -> GetFixtureA() -> GetBody() == play[i].m_wheel1 ||
					contact -> GetFixtureA() -> GetBody() == play[i].m_wheel2 ||
					contact -> GetFixtureA() -> GetBody() == play[i].m_wheel3 ||
					contact -> GetFixtureA() -> GetBody() == play[i].m_wheel4 ||
					contact -> GetFixtureA() -> GetBody() == play[i].m_wheel5)
					
				{
					defeat[i] = true; //waits till contact is complete, queues for destruction
					//destroy(i);
					break;
				}
			}
			if(!flag)
			{
				nextTurn();
			}
		}
	}
	void Keyboard(unsigned char key)
	{
		if(!gameOver) //can't move after game ends
		{
			switch (key)
			{
			case 'a':
				if(fuel[turn] > 0)
				{
					play[turn].m_spring1->SetMotorSpeed(m_speed);
					play[turn].m_spring2->SetMotorSpeed(m_speed);
					play[turn].m_spring3->SetMotorSpeed(m_speed);
					play[turn].m_spring4->SetMotorSpeed(m_speed);
					play[turn].m_spring5->SetMotorSpeed(m_speed);
					fuel[turn]--;
				}
				break;

			case 'd':
				if(fuel[turn > 0])
				{
					play[turn].m_spring1->SetMotorSpeed(-m_speed);
					play[turn].m_spring2->SetMotorSpeed(-m_speed);
					play[turn].m_spring3->SetMotorSpeed(-m_speed);
					play[turn].m_spring4->SetMotorSpeed(-m_speed);
					play[turn].m_spring5->SetMotorSpeed(-m_speed);
					fuel[turn]--;
				}
				break;
			case 'q':
				play[turn].m_rotate->SetMotorSpeed(m_rspeed);
				break;
			case 'e':
				play[turn].m_rotate->SetMotorSpeed(-m_rspeed);
				break;
			case 's':
				if(vel[turn] > MIN_VEL) vel[turn] -= 5;
				break;
			case 'w':
				if(vel[turn] < MAX_VEL) vel[turn] += 5;
				break;

			case ' ': //shoots bullet
				if (m_bullet == NULL) //only one bullet at a time
				{
					stepCount = 0;
					b2CircleShape shape;
					shape.m_radius = 0.25f;

					b2BodyDef bd;
					bd.type = b2_dynamicBody;
					bd.position.Set(play[turn].m_tank -> GetPosition().x + (cosf(ang) * 1.6f), //sets it so it doesnt collide with cannon
					play[turn].m_tank -> GetPosition().y + (sinf(ang) * 1.6f) + 0.6f);
										
					m_bullet = m_world->CreateBody(&bd);
					bd.bullet = true;

					ang = play[turn].m_cannon -> GetAngle();

					// shape.m_p.Set(play[turn].m_tank -> GetPosition().x + (cosf(ang) * 1.6f), //sets it so it doesnt collide with cannon
					// play[turn].m_tank -> GetPosition().y + (sinf(ang) * 1.6f) + 0.6f);
				

					b2Fixture *f =  m_bullet -> CreateFixture(&shape, 5.0f);
					f->SetUserData((void*)1);//enables collision

					m_bullet -> SetLinearVelocity(b2Vec2(vel[turn] * cosf(ang) / 3, vel[turn] * sinf(ang) / 3));
				}
				break;
			case '/'://for testing use
				defeat[turn] = true;
				nextTurn();
				break;
			case '.':
				nextTurn();
				break;
			default:
				stopMotors();
			break;
			}
		}
	}
	void KeyboardUp(unsigned char key) //stops after release
	{
		switch (key)
		{
		case 'a':
		case 'd':
		case 'q':
		case 'e':
			stopMotors();
			break;
		}
	}

	virtual void Step(Settings *settings)
	{
		m_debugDraw.DrawString(5, m_textLine, "left/right = a|d     aim = q|e     power = s|w");
		m_textLine += DRAW_STRING_NEW_LINE;
		m_debugDraw.DrawString(5, m_textLine, "Fire = Space     Player %u's turn     Players remaining: %u", turn + 1, remPlayers);
		m_textLine += DRAW_STRING_NEW_LINE;
		m_debugDraw.DrawString(5, m_textLine, "Fuel %u     Power = %u", fuel[turn], vel[turn]);
		stepCount ++;
		
		if(ded[turn])
		{
			nextTurn();
		}

		if(stepCount == 300 && m_bullet != NULL) //lost Bullet
		{	
			m_world -> DestroyBody(m_bullet);
			m_bullet = NULL;
			nextTurn();
		}

		for(i = 0; i < numPlayers; i ++) //can
		{
			if (defeat[i])
			{
				destroy(i);
			}
		}

		const uint32 particleFlags = TestMain::GetParticleParameterValue();
		//Test::Step(settings);
		// settings -> viewCenter.x = play[turn].m_tank->GetPosition().x;
		// settings -> viewCenter.y = play[turn].m_tank->GetPosition().y;
		if(m_bullet != NULL) //camera pan
		{
			settings -> viewCenter.x = m_bullet -> GetPosition().x;
		} 
		else 
		{
			settings -> viewCenter.x = play[turn].m_tank->GetPosition().x;
			settings -> viewCenter.y = play[turn].m_tank->GetPosition().y + 15;
		}

		if (m_contact) //creates particle explosions
		{
			m_bullet = NULL;
			AddVFX(m_contactPoint, particleFlags);
			m_contact = false;
		}

		for (int i = 0; i < c_maxVFX; i++) //particles
		{
			PVFX *vfx = m_VFX[i];
			if (vfx == NULL)
				continue;
			vfx->Step(1.0f/settings->hz);
			if (vfx->IsDone())
			{
				delete vfx;
				m_VFX[i] = NULL;
			}
		}


		if(gameOver)
		{
			m_textLine += DRAW_STRING_NEW_LINE;
			m_debugDraw.DrawString(5, m_textLine, "GAME OVER! Winner: Player %u", turn + 1);
			//AddVFX(b2Vec2(10.0f, 10.0f), particleFlags);//victory particles
		}


		Test::Step(settings);
	}

	static Test* Create()
	{
		return new Tanks;
	}

	int i, turn, numPlayers, remPlayers, len,  stepCount, vel[MAX_PLAYERS],fuel[MAX_PLAYERS];
	bool defeat[MAX_PLAYERS], ded[MAX_PLAYERS], gameOver, flag;
	float32 ang, m_speed, m_rspeed, leftBound, rightBound, startPos[MAX_PLAYERS];
    b2Body* m_bullet;

private:
	int m_VFXIndex;
	static const int c_maxVFX = 100;
	PVFX *m_VFX[c_maxVFX];

	bool m_contact;
	b2Vec2 m_contactPoint;
};
#endif