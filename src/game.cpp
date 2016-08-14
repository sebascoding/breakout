/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <ctime>

#include <irrKlang.h>
using namespace irrklang;

#include "game.hpp"
#include "resource_manager.hpp"
#include "sprite_renderer.hpp"
#include "game_object.hpp"
#include "ball_object.hpp"
#include "particle_generator.hpp"
#include "post_processor.hpp"
#include "text_renderer.hpp"


// Game-related State data
SpriteRenderer				*Renderer;
GameObject					*Player;
std::vector<BallObject *>	Balls;
std::map<BallObject *, ParticleGenerator *>					ballParticle;
PostProcessor				*Effects;
ISoundEngine				*SoundEngine = createIrrKlangDevice();
GLfloat						ShakeTime = 0.0f;
TextRenderer				*Text;
GLuint						BricksLeft;


void AddBall(BallObject *ball);
std::vector<BallObject *>::iterator RemoveBall(BallObject *ball);

Game::Game(GLuint width, GLuint height) 
    : State(GAME_MENU), Keys(), Width(width), Height(height), Level(0), Lives(3), Score(0)
{ 

}

Game::~Game()
{
    delete Renderer;
    delete Player;
	for (std::vector<BallObject *>::iterator it = Balls.begin(); it != Balls.end();)
		it = RemoveBall((*it));
    delete Effects;
    delete Text;
    SoundEngine->drop();
}

void Game::Init()
{
    // Load shaders
    ResourceManager::LoadShader("shaders/sprite.vert", "shaders/sprite.frag", nullptr, "sprite");
    ResourceManager::LoadShader("shaders/particle.vert", "shaders/particle.frag", nullptr, "particle");
    ResourceManager::LoadShader("shaders/post_processing.vert", "shaders/post_processing.frag", nullptr, "postprocessing");
    // Configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(this->Width), static_cast<GLfloat>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);
    // Load textures
    ResourceManager::LoadTexture("assets/textures/background.jpg", GL_FALSE, "background");
    ResourceManager::LoadTexture("assets/textures/awesomeface.png", GL_TRUE, "face");
    ResourceManager::LoadTexture("assets/textures/block.png", GL_FALSE, "block");
    ResourceManager::LoadTexture("assets/textures/block_solid.png", GL_FALSE, "block_solid");
    ResourceManager::LoadTexture("assets/textures/paddle.png", GL_TRUE, "paddle");
    ResourceManager::LoadTexture("assets/textures/particle.png", GL_TRUE, "particle");
    ResourceManager::LoadTexture("assets/textures/powerup_speed.png", GL_TRUE, "powerup_speed");
    ResourceManager::LoadTexture("assets/textures/powerup_sticky.png", GL_TRUE, "powerup_sticky");
    ResourceManager::LoadTexture("assets/textures/powerup_increase.png", GL_TRUE, "powerup_increase");
    ResourceManager::LoadTexture("assets/textures/powerup_confuse.png", GL_TRUE, "powerup_confuse");
    ResourceManager::LoadTexture("assets/textures/powerup_chaos.png", GL_TRUE, "powerup_chaos");
    ResourceManager::LoadTexture("assets/textures/powerup_passthrough.png", GL_TRUE, "powerup_passthrough");
	ResourceManager::LoadTexture("assets/textures/powerup_decrease.png", GL_TRUE, "powerup_decrease");
	ResourceManager::LoadTexture("assets/textures/powerup_bigball.png", GL_TRUE, "powerup_bigball");
	ResourceManager::LoadTexture("assets/textures/powerup_multiball.png", GL_TRUE, "powerup_multiball");
    // Set render-specific controls
    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    Effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);
    Text = new TextRenderer(this->Width, this->Height);
    Text->Load("assets/fonts/ocraext.ttf", 24);
    // Load levels
    GameLevel one; one.Load("assets/levels/one.lvl", this->Width, this->Height * 0.5);
    GameLevel two; two.Load("assets/levels/two.lvl", this->Width, this->Height * 0.5);
    GameLevel three; three.Load("assets/levels/three.lvl", this->Width, this->Height * 0.5);
    GameLevel four; four.Load("assets/levels/four.lvl", this->Width, this->Height * 0.5);
    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);
    this->Level = 0;
	BricksLeft = this->Levels[this->Level].CountBlocks(GL_FALSE);
    // Configure game objects
    glm::vec2 playerPos = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));
    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -BALL_RADIUS * 2);
	BallObject *ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("face"));
	AddBall(ball);
    // Audio
    SoundEngine->play2D("assets/audio/breakout.mp3", GL_TRUE);
}

void Game::Update(GLfloat dt)
{
    // Update objects
	for (BallObject *Ball : Balls)
		Ball->Move(dt, this->Width);
    // Check for collisions
    this->DoCollisions();
    // Update particles	
	for (BallObject *Ball : Balls)
		if (!Ball->Stuck)
			ballParticle[Ball]->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2));
    // Update PowerUps
    this->UpdatePowerUps(dt);
    // Reduce shake time
    if (ShakeTime > 0.0f)
    {
        ShakeTime -= dt;
        if (ShakeTime <= 0.0f)
            Effects->Shake = GL_FALSE;
    }
    // Check loss condition
	GLfloat height = this->Height;

	for (std::vector<BallObject *>::iterator it = Balls.begin(); it != Balls.end();)
	{
		if ((*it)->Position.y >= this->Height)
		{
			it = RemoveBall((*it));
		}
		else
			++it;
	}
	//Balls.erase(std::remove_if(Balls.begin(), Balls.end(), [height](BallObject *ball) { return ball->Position.y >= height; }), Balls.end());

	if (Balls.size() == 0) // Did ball reach bottom edge?
	{
		--this->Lives;
		// Did the player lose all his lives? : Game over
		if (this->Lives == 0)
		{
			this->ResetLevel();
			this->State = GAME_MENU;
		}
		this->ResetPlayer();
	}

    // Check win condition
    if (this->State == GAME_ACTIVE && this->Levels[this->Level].IsCompleted())
    {
        this->ResetLevel();
        this->ResetPlayer();
        Effects->Chaos = GL_TRUE;
        this->State = GAME_WIN;
    }
}


void Game::ProcessInput(GLfloat dt)
{
    if (this->State == GAME_MENU)
    {
        if (this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER])
        {
            this->State = GAME_ACTIVE;
			this->Score = 0;
            this->KeysProcessed[GLFW_KEY_ENTER] = GL_TRUE;
        }
        if (this->Keys[GLFW_KEY_W] && !this->KeysProcessed[GLFW_KEY_W])
        {
            this->Level = (this->Level + 1) % 4;
			BricksLeft = this->Levels[this->Level].CountBlocks(GL_FALSE);
            this->KeysProcessed[GLFW_KEY_W] = GL_TRUE;
        }
        if (this->Keys[GLFW_KEY_S] && !this->KeysProcessed[GLFW_KEY_S])
        {
            if (this->Level > 0)
                --this->Level;
            else
                this->Level = 3;
			BricksLeft = this->Levels[this->Level].CountBlocks(GL_FALSE);
            this->KeysProcessed[GLFW_KEY_S] = GL_TRUE;
        }
    }
    if (this->State == GAME_WIN)
    {
        if (this->Keys[GLFW_KEY_ENTER])
        {
            this->KeysProcessed[GLFW_KEY_ENTER] = GL_TRUE;
            Effects->Chaos = GL_FALSE;
            this->State = GAME_MENU;
        }
    }
    if (this->State == GAME_ACTIVE)
    {
        GLfloat velocity = PLAYER_VELOCITY * dt;
        // Move playerboard
        if (this->Keys[GLFW_KEY_A])
        {
            if (Player->Position.x >= 0)
            {
                Player->Position.x -= velocity;
				for (BallObject *Ball : Balls)
					if (Ball->Stuck)
						Ball->Position.x -= velocity;
            }
        }
        if (this->Keys[GLFW_KEY_D])
        {
            if (Player->Position.x <= this->Width - Player->Size.x)
            {
                Player->Position.x += velocity;
				for (BallObject *Ball : Balls)
					if (Ball->Stuck)
						Ball->Position.x += velocity;
            }
        }
        if (this->Keys[GLFW_KEY_SPACE])
			for (BallObject *Ball : Balls)
				Ball->Stuck = GL_FALSE;
    }
}

void Game::Render()
{
    if (this->State == GAME_ACTIVE || this->State == GAME_MENU || this->State == GAME_WIN)
    {
        // Begin rendering to postprocessing quad
        Effects->BeginRender();
            // Draw background
            Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0, 0), glm::vec2(this->Width, this->Height), 0.0f);
            // Draw level
            this->Levels[this->Level].Draw(*Renderer);
            // Draw player
            Player->Draw(*Renderer);
            // Draw PowerUps
            for (PowerUp &powerUp : this->PowerUps)
                if (!powerUp.Destroyed)
                    powerUp.Draw(*Renderer);
            // Draw particles	
			for (BallObject *Ball : Balls)
				if (!Ball->Stuck)
					ballParticle[Ball]->Draw();
            // Draw ball
			for (BallObject *Ball : Balls)
				Ball->Draw(*Renderer);            
        // End rendering to postprocessing quad
        Effects->EndRender();
        // Render postprocessing quad
        Effects->Render(glfwGetTime());
        // Render text (don't include in postprocessing)
        std::stringstream sLives; sLives << this->Lives;
		std::stringstream sScore; sScore << this->Score;
		std::stringstream sBricks; sBricks << BricksLeft;
        Text->RenderText("Lives:" + sLives.str(), 5.0f, 5.0f, 1.0f);
		Text->RenderText("Score:" + sScore.str(), this->Width/2 - 50, 5.0f, 1.0f);
		Text->RenderText("Bricks left:" + sBricks.str(), this->Width - 230, 5.0f, 1.0f);
    }
    if (this->State == GAME_MENU)
    {
        Text->RenderText("Press ENTER to start", 250.0f, this->Height / 2, 1.0f);
        Text->RenderText("Press W or S to select level", 245.0f, this->Height / 2 + 20.0f, 0.75f);
    }
    if (this->State == GAME_WIN)
    {
        Text->RenderText("You WON!!!", 320.0f, this->Height / 2 - 20.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        Text->RenderText("Press ENTER to retry or ESC to quit", 130.0f, this->Height / 2, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
    }
}


void Game::ResetLevel()
{
    if (this->Level == 0)this->Levels[0].Load("assets/levels/one.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 1)
        this->Levels[1].Load("assets/levels/two.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 2)
        this->Levels[2].Load("assets/levels/three.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 3)
        this->Levels[3].Load("assets/levels/four.lvl", this->Width, this->Height * 0.5f);

    this->Lives = 3;
	BricksLeft = this->Levels[this->Level].CountBlocks(GL_FALSE);
}

void Game::ResetPlayer()
{
    // Reset player/ball stats
    Player->Size = PLAYER_SIZE;
    Player->Position = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);

	glm::vec2 ballPos = Player->Position + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -BALL_RADIUS * 2);
	for (std::vector<BallObject *>::iterator it = Balls.begin(); it != Balls.end();)
		it = RemoveBall((*it));

	BallObject *newball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("face"));
	AddBall(newball);

    Effects->Chaos = Effects->Confuse = GL_FALSE;
    Player->Color = glm::vec3(1.0f);

	this->ClearPowerUps();
	
	for (std::map<BallObject *, ParticleGenerator *>::iterator ii = ballParticle.begin(); ii != ballParticle.end(); ++ii)
		(*ii).second->Reset();
}


// PowerUps
GLboolean IsOtherPowerUpActive(std::vector<PowerUp> &powerUps, std::string type);

void Game::UpdatePowerUps(GLfloat dt)
{
    for (PowerUp &powerUp : this->PowerUps)
    {
        powerUp.Position += powerUp.Velocity * dt;
        if (powerUp.Activated)
        {
            powerUp.Duration -= dt;

            if (powerUp.Duration <= 0.0f)
            {
                // Remove powerup from list (will later be removed)
                powerUp.Activated = GL_FALSE;
                // Deactivate effects
                if (powerUp.Type == "sticky")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "sticky"))
                    {	// Only reset if no other PowerUp of type sticky is active
						for (BallObject *Ball : Balls)
							Ball->Sticky = GL_FALSE;
                        Player->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerUp.Type == "pass-through")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "pass-through"))
                    {	// Only reset if no other PowerUp of type pass-through is active
						for (BallObject *Ball : Balls)
						{
							Ball->PassThrough = GL_FALSE;
							Ball->Color = glm::vec3(1.0f);
						}
                    }
                }
				else if (powerUp.Type == "pad-size-increase")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "pad-size-increase"))
					{
						Player->Size = PLAYER_SIZE;
					}
				}
				else if (powerUp.Type == "ball-big")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "ball-big"))
					{
						for (BallObject *Ball : Balls)
						{
							Ball->Resize(BALL_RADIUS);
						}
					}
				}
				else if (powerUp.Type == "pad-size-decrease")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "pad-size-decrease"))
					{
						Player->Size = PLAYER_SIZE;
					}
				}
                else if (powerUp.Type == "confuse")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "confuse"))
                    {	// Only reset if no other PowerUp of type confuse is active
                        Effects->Confuse = GL_FALSE;
                    }
                }
                else if (powerUp.Type == "chaos")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "chaos"))
                    {	// Only reset if no other PowerUp of type chaos is active
                        Effects->Chaos = GL_FALSE;
                    }
                }				
            }
        }
    }
    // Remove all PowerUps from vector that are destroyed AND !activated (thus either off the map or finished)
    // Note we use a lambda expression to remove each PowerUp which is destroyed and not activated
    this->PowerUps.erase(std::remove_if(this->PowerUps.begin(), this->PowerUps.end(),
        [](const PowerUp &powerUp) { return powerUp.Destroyed && !powerUp.Activated; }
    ), this->PowerUps.end());
}

void Game::ClearPowerUps()
{
	PowerUps.clear();
}

GLboolean ShouldSpawn(GLuint chance)
{
    GLuint random = rand() % chance;
    return random == 0;
}

glm::vec2 PowerUpVelocity() {
	srand(static_cast <unsigned> (time(0)));
	float r = static_cast <float> (rand() % 10) / 10.0f + 0.5f;
	return VELOCITY * (r);
}
void Game::SpawnPowerUps(GameObject &block)
{
    if (ShouldSpawn(75)) // 1 in 75 chance
        this->PowerUps.push_back(PowerUp("speed", glm::vec3(0.5f, 0.5f, 1.0f), 0.0f, block.Position, ResourceManager::GetTexture("powerup_speed"), VELOCITY * 1.5f));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("sticky", glm::vec3(1.0f, 0.5f, 1.0f), 20.0f, block.Position, ResourceManager::GetTexture("powerup_sticky"), VELOCITY * 1.5f));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("pass-through", glm::vec3(0.5f, 1.0f, 0.5f), 10.0f, block.Position, ResourceManager::GetTexture("powerup_passthrough"), VELOCITY * 1.5f));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("pad-size-increase", glm::vec3(1.0f, 0.6f, 0.4f), 10.0f, block.Position, ResourceManager::GetTexture("powerup_increase"), VELOCITY * 1.5f));
	if (ShouldSpawn(75))
		this->PowerUps.push_back(PowerUp("ball-big", glm::vec3(0.15f, 0.55f, 0.15f), 10.0f, block.Position, ResourceManager::GetTexture("powerup_bigball"), VELOCITY * 1.5f));
	if (ShouldSpawn(2))
		this->PowerUps.push_back(PowerUp("ball-multi", glm::vec3(0.15f, 0.55f, 0.15f), 0.0f, block.Position, ResourceManager::GetTexture("powerup_multiball"), VELOCITY * 1.5f));
	if (ShouldSpawn(15))
		this->PowerUps.push_back(PowerUp("pad-size-decrease", glm::vec3(0.8f, 0.6f, 0.2f), 20.0f, block.Position, ResourceManager::GetTexture("powerup_decrease")));
    if (ShouldSpawn(15)) // Negative powerups should spawn more often
        this->PowerUps.push_back(PowerUp("confuse", glm::vec3(1.0f, 0.3f, 0.3f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_confuse")));
    if (ShouldSpawn(15))
        this->PowerUps.push_back(PowerUp("chaos", glm::vec3(0.9f, 0.25f, 0.25f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_chaos")));
}

void ActivatePowerUp(PowerUp &powerUp)
{
    // Initiate a powerup based type of powerup
    if (powerUp.Type == "speed")
    {
		for (BallObject *Ball : Balls)
			Ball->Velocity *= 1.2;
    }
    else if (powerUp.Type == "sticky")
    {
		for (BallObject *Ball : Balls)
			Ball->Sticky = GL_TRUE;
        Player->Color = glm::vec3(1.0f, 0.5f, 1.0f);
    }
    else if (powerUp.Type == "pass-through")
    {
		for (BallObject *Ball : Balls)
		{
			Ball->PassThrough = GL_TRUE;
			Ball->Color = glm::vec3(1.0f, 0.5f, 0.5f);
		}
    }
    else if (powerUp.Type == "pad-size-increase")
    {
        Player->Size.x += 50;
    }
	else if (powerUp.Type == "ball-big")
	{
		for (BallObject *Ball : Balls)
			Ball->Resize(BALL_RADIUS * 2);
	}
	else if (powerUp.Type == "ball-multi")
	{		
		BallObject *newball = Balls[0]->clone();
		newball->Stuck = GL_FALSE;
		newball->Position = Player->Position + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -BALL_RADIUS * 2);
		newball->Velocity = glm::vec2(-newball->Velocity.x, -glm::abs(newball->Velocity.y));
		Balls.push_back(newball);
		ballParticle[newball] = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), PARTICLE_AMOUNT);
	}
	else if (powerUp.Type == "pad-size-decrease")
	{
		Player->Size.x -= 50;
		if (Player->Size.x < 50)
			Player->Size.x = 50;
	}
    else if (powerUp.Type == "confuse")
    {
        if (!Effects->Chaos)
            Effects->Confuse = GL_TRUE; // Only activate if chaos wasn't already active
    }
    else if (powerUp.Type == "chaos")
    {
        if (!Effects->Confuse)
            Effects->Chaos = GL_TRUE;
    }

}

GLboolean IsOtherPowerUpActive(std::vector<PowerUp> &powerUps, std::string type)
{
    // Check if another PowerUp of the same type is still active
    // in which case we don't disable its effect (yet)
    for (const PowerUp &powerUp : powerUps)
    {
        if (powerUp.Activated)
            if (powerUp.Type == type)
                return GL_TRUE;
    }
    return GL_FALSE;
}


// Collision detection
GLboolean CheckCollision(GameObject &one, GameObject &two);
Collision CheckCollision(BallObject &one, GameObject &two);
Direction VectorDirection(glm::vec2 closest);

void Game::DoCollisions()
{
    for (GameObject &box : this->Levels[this->Level].Bricks)
    {
        if (!box.Destroyed)
        {
			for (BallObject *Ball : Balls)
			{
				Collision collision = CheckCollision(*Ball, box);
				if (std::get<0>(collision)) // If collision is true
				{
					// Destroy block if not solid
					if (!box.IsSolid)
					{
						box.Destroyed = GL_TRUE;
						this->SpawnPowerUps(box);
						SoundEngine->play2D("assets/audio/bleep.mp3", GL_FALSE);
						BricksLeft--;
						this->Score += 3;
					}
					else
					{   // if block is solid, enable shake effect
						ShakeTime = 0.05f;
						Effects->Shake = GL_TRUE;
						SoundEngine->play2D("assets/audio/solid.wav", GL_FALSE);
					}
					// Collision resolution
					Direction dir = std::get<1>(collision);
					glm::vec2 diff_vector = std::get<2>(collision);
					if (!(Ball->PassThrough && !box.IsSolid)) // don't do collision resolution on non-solid bricks if pass-through activated
					{
						if (dir == LEFT || dir == RIGHT) // Horizontal collision
						{
							Ball->Velocity.x = -Ball->Velocity.x; // Reverse horizontal velocity
							// Relocate
							GLfloat penetration = Ball->Radius - std::abs(diff_vector.x);
							if (dir == LEFT)
								Ball->Position.x += penetration; // Move ball to right
							else
								Ball->Position.x -= penetration; // Move ball to left;
						}
						else // Vertical collision
						{
							Ball->Velocity.y = -Ball->Velocity.y; // Reverse vertical velocity
							// Relocate
							GLfloat penetration = Ball->Radius - std::abs(diff_vector.y);
							if (dir == UP)
								Ball->Position.y -= penetration; // Move ball bback up
							else
								Ball->Position.y += penetration; // Move ball back down
						}
					}
				}
			}
        }    
    }

    // Also check collisions on PowerUps and if so, activate them
    for (PowerUp &powerUp : this->PowerUps)
    {
        if (!powerUp.Destroyed)
        {
            // First check if powerup passed bottom edge, if so: keep as inactive and destroy
            if (powerUp.Position.y >= this->Height)
                powerUp.Destroyed = GL_TRUE;

            if (CheckCollision(*Player, powerUp))
            {	// Collided with player, now activate powerup
                ActivatePowerUp(powerUp);
                powerUp.Destroyed = GL_TRUE;
                powerUp.Activated = GL_TRUE;
                SoundEngine->play2D("assets/audio/powerup.wav", GL_FALSE);
            }
        }
    }

    // And finally check collisions for player pad (unless stuck)
	for (BallObject *Ball : Balls)
	{
		Collision result = CheckCollision(*Ball, *Player);
		if (!Ball->Stuck && std::get<0>(result))
		{
			// Check where it hit the board, and change velocity based on where it hit the board
			GLfloat centerBoard = Player->Position.x + Player->Size.x / 2;
			GLfloat distance = (Ball->Position.x + Ball->Radius) - centerBoard;
			GLfloat percentage = distance / (Player->Size.x / 2);
			// Then move accordingly
			GLfloat strength = 2.0f;
			glm::vec2 oldVelocity = Ball->Velocity;
			Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
			//Ball->Velocity.y = -Ball->Velocity.y;
			Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity); // Keep speed consistent over both axes (multiply by length of old velocity, so total strength is not changed)
			// Fix sticky paddle
			Ball->Velocity.y = -1 * abs(Ball->Velocity.y);

			// If Sticky powerup is activated, also stick ball to paddle once new velocity vectors were calculated
			Ball->Stuck = Ball->Sticky;

			SoundEngine->play2D("assets/audio/bleep.wav", GL_FALSE);
		}
	}
}

GLboolean CheckCollision(GameObject &one, GameObject &two) // AABB - AABB collision
{
    // Collision x-axis?
    GLboolean collisionX = one.Position.x + one.Size.x >= two.Position.x &&
        two.Position.x + two.Size.x >= one.Position.x;
    // Collision y-axis?
    GLboolean collisionY = one.Position.y + one.Size.y >= two.Position.y &&
        two.Position.y + two.Size.y >= one.Position.y;
    // Collision only if on both axes
    return collisionX && collisionY;
}

Collision CheckCollision(BallObject &one, GameObject &two) // AABB - Circle collision
{
    // Get center point circle first 
    glm::vec2 center(one.Position + one.Radius);
    // Calculate AABB info (center, half-extents)
    glm::vec2 aabb_half_extents(two.Size.x / 2, two.Size.y / 2);
    glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
    // Get difference vector between both centers
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    // Now that we know the the clamped values, add this to AABB_center and we get the value of box closest to circle
    glm::vec2 closest = aabb_center + clamped;
    // Now retrieve vector between center circle and closest point AABB and check if length < radius
    difference = closest - center;
    
    if (glm::length(difference) < one.Radius) // not <= since in that case a collision also occurs when object one exactly touches object two, which they are at the end of each collision resolution stage.
        return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
    else
        return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
}

// Calculates which direction a vector is facing (N,E,S or W)
Direction VectorDirection(glm::vec2 target)
{
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),	// up
        glm::vec2(1.0f, 0.0f),	// right
        glm::vec2(0.0f, -1.0f),	// down
        glm::vec2(-1.0f, 0.0f)	// left
    };
    GLfloat max = 0.0f;
    GLuint best_match = -1;
    for (GLuint i = 0; i < 4; i++)
    {
        GLfloat dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max)
        {
            max = dot_product;
            best_match = i;
        }
    }
    return (Direction)best_match;
}

void AddBall(BallObject *ball)
{
	Balls.push_back(ball);
	if (ballParticle.find(ball) == ballParticle.end())
		ballParticle[ball] = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), PARTICLE_AMOUNT);
	else
		ballParticle[ball]->Reset();
}

std::vector<BallObject *>::iterator RemoveBall(BallObject *ball)
{
	if (ballParticle.find(ball) != ballParticle.end())
	{
		delete ballParticle[ball];
		ballParticle.erase(ball);
	}

	auto it = std::find_if(Balls.begin(), Balls.end(), [ball](const BallObject *_ball) { return ball == _ball; });

	if (it != Balls.end())
		return Balls.erase(it);
	else
		throw;
}