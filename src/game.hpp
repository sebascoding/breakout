/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#ifndef GAME_H
#define GAME_H
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "game_level.hpp"
#include "powerup.hpp"

// Represents the current state of the game
enum GameState {
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN
};

// Represents the four possible (collision) directions
enum Direction {
	UP,
	RIGHT,
	DOWN,
	LEFT
};
// Defines a Collision typedef that represents collision data
typedef std::tuple<GLboolean, Direction, glm::vec2> Collision; // <collision?, what direction?, difference vector center - closest point>

// Initial size of the player paddle
const glm::vec2 PLAYER_SIZE(100, 20);
// Initial velocity of the player paddle
const GLfloat PLAYER_VELOCITY(500.0f);
// Initial velocity of the Ball
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);
// Radius of the ball object
const GLfloat BALL_RADIUS = 12.5f;
// Amount of ball particles
const GLuint PARTICLE_AMOUNT = 500;

// Game holds all game-related state and functionality.
// Combines all game-related data into a single class for
// easy access to each of the components and manageability.
class Game
{
public:
    // Game state
	GLuint				   Lives;
    GameState              State;	
    GLboolean              Keys[1024];
	GLboolean			   KeysProcessed[1024];
    GLuint                 Width, Height;
    std::vector<GameLevel> Levels;
    GLuint                 Level;
	std::vector<PowerUp>  PowerUps;
    // Constructor/Destructor
    Game(GLuint width, GLuint height);
    ~Game();
    // Initialize game state (load all shaders/textures/levels)
    void Init();
	// Resize Game window
	void Resize(GLuint width, GLuint height) { this->Width = width; this->Height = height; }
    // GameLoop
    void ProcessInput(GLfloat dt);
    void Update(GLfloat dt);
    void Render();
	void DoCollisions();
	// Reset
	void ResetLevel();
	void ResetPlayer();
	//PowerUps
	void SpawnPowerUps(GameObject &block);
	void UpdatePowerUps(GLfloat dt);
};

#endif