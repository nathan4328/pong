/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#include <algorithm>
#include <iostream>
#include <random>
#include <ctime>
#include <unistd.h>

#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "game_object.h"
#include "ball_object.h"
#include "text_renderer.h"
#include <irrklang/irrKlang.h>
using namespace irrklang;


// Game-related State data
SpriteRenderer    *Renderer;
GameObject        *Player;
GameObject        *Opponent;
BallObject        *Ball;
TextRenderer      *Text;
ISoundEngine      *SoundEngine = createIrrKlangDevice();

glm::vec2 ballVelocity;

Game::Game(unsigned int width, unsigned int height) 
    : State(GAME_MENU), Keys(), Width(width), Height(height)
{ 

}

Game::~Game()
{
    delete Renderer;
    delete Player;
    delete Ball;
    delete Text;
    SoundEngine->drop();
    delete SoundEngine;
}

void Game::Init()
{
    // load shaders
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.fs", nullptr, "sprite");
    // configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width), 
        static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);    
    // load textures
    ResourceManager::LoadTexture("textures/background.jpg", false, "background");
    ResourceManager::LoadTexture("textures/awesomeface.png", true, "face");
    ResourceManager::LoadTexture("textures/block.png", false, "block");
    ResourceManager::LoadTexture("textures/block_solid.png", false, "block_solid");
    ResourceManager::LoadTexture("textures/paddle.png", true, "paddle");
    ResourceManager::LoadTexture("textures/particle.png", true, "particle");

    // set render-specific controls
    Shader spriteShader = ResourceManager::GetShader("sprite");
    Renderer = new SpriteRenderer(spriteShader);
    Text = new TextRenderer(this->Width, this->Height);
    Text->Load("fonts/OCRAEXT.TTF", 24);

    // configure game objects

    float paddleRotation = 0;
    glm::vec2 playerPos = glm::vec2(0, this->Height /2.0f  - PLAYER_SIZE.y/2);
    //Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));
    Player = new GameObject(playerPos, PLAYER_SIZE, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 1.0f), paddleRotation, 
                                ResourceManager::GetTexture("paddle"), true, false);
    glm::vec2 opponentPos = glm::vec2(this->Width-PLAYER_SIZE.x, this->Height/2.0f - PLAYER_SIZE.y/2);
    Opponent = new GameObject(opponentPos,PLAYER_SIZE, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), paddleRotation, 
                                ResourceManager::GetTexture("paddle"), true, false);


   std::srand(std::time(nullptr));

    // Constants
    const float PI = 3.14159265f;
    const float strength = 1000.0f;

    // Generate a random angle between 0 and 2*PI
    float randomAngle = (std::rand() % 360) * PI / 180.0f; 

    // Calculate velocity components using trigonometry
    float randomX = strength * cos(randomAngle); 
    float randomY = strength * sin(randomAngle);

    // Create the velocity vector
    ballVelocity = glm::vec2(randomX + 100.0f, randomY + 100.0f);
    Ball = new BallObject(glm::vec2(this->Width/2, this->Height/2), BALL_RADIUS, ballVelocity, ResourceManager::GetTexture("face"));
    Ball->Color = glm::vec3(1.0f, 0.0f, 0.0f);

    playerScore = 0;
    opponentScore = 0;
    isUpdating = false;

    SoundEngine->play2D("audio/breakout.mp3", true);

}

void Game::Update(float dt)
{
    // update objects
     // check for collisions
    //this->DoCollisions();

    if(this->State == GAME_ACTIVE)
    {
        Ball->Move(dt, this->Width, this->Height );

        this->DoCollisions();

        if(Ball->Position.x <= 0 && !isUpdating)
    {
        opponentScore++;
        isUpdating = true;
        if(opponentScore >= 10)
        {
            this->State = GAME_LOSS;
            ResetGame();
        }
        ResetGame();
    }

        if(Ball->Position.x >= this->Width && !isUpdating)
        {
            playerScore++;
            isUpdating = true;
            if(playerScore >= 10)
            {
                this->State = GAME_WIN;

                ResetGame();
            }
            ResetGame();
        }
    }


    
}

void Game::ProcessInput(float dt)
{
    if (this->State == GAME_MENU)
    {
        if (this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER])
        {
            this->State = GAME_ACTIVE;
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
        }
    }
    if(this->State == GAME_LOSS)
    {
        if (this->Keys[GLFW_KEY_ENTER])
        {
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
            this->State = GAME_MENU;
        }
    }
    if(this->State == GAME_WIN)
    {
        if (this->Keys[GLFW_KEY_ENTER])
        {
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
            this->State = GAME_MENU;
        }
    }
    if (this->State == GAME_ACTIVE)
    {
        float velocity = PLAYER_VELOCITY * dt;
        // move playerboard
        if (this->Keys[GLFW_KEY_W])
        {
            if (Player->Position.y >= 0.0f)
            {
                Player->Position.y -= velocity;
            }
        }
        if (this->Keys[GLFW_KEY_S])
        {
            if (Player->Position.y <= this->Height - Player->Size.y)
            {
                Player->Position.y += velocity;
            }
        }
        if (this->Keys[GLFW_KEY_I])
        {
            if (Opponent->Position.y >= 0.0f)
            {
                Opponent->Position.y -= velocity;
            }
        }
        if (this->Keys[GLFW_KEY_K])
        {
            if (Opponent->Position.y <= this->Height - Player->Size.y)
            {
                Opponent->Position.y += velocity;
            }
        }
    }
}

void Game::Render(float dt)
{
    if(this->State == GAME_ACTIVE)
    {
        // draw background
        Texture2D backgroundTex = ResourceManager::GetTexture("background");
        Renderer->DrawSprite(backgroundTex, glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
        // draw player
        Player->Draw(*Renderer);   
        Opponent->Draw(*Renderer);
        // end rendering to postprocessing framebuffer
        Ball->Draw(*Renderer);
        // render postprocessing quad
        std::string score = "Score:" +std::to_string(playerScore) + " - " +std::to_string(opponentScore);
        Text->RenderText(score, this->Width /2 -110.0f, 5.0f, 1.0f);
    }
    if (this->State == GAME_MENU)
    {
        Text->RenderText("Press ENTER to start", 250.0f, this->Height / 2.0f, 1.0f);
        playerScore = 0;
        opponentScore = 0;
        // draw player
        Player->Draw(*Renderer);   
        Opponent->Draw(*Renderer);
        // end rendering to postprocessing framebuffer
        Ball->Draw(*Renderer);
    }
    if (this->State == GAME_WIN)
    {
        Text->RenderText("Player 1 WON!!!", this->Width-20.0f, this->Height / 2.0f - 20.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        Text->RenderText("Score: " + std::to_string(playerScore) +" - "+ std::to_string(opponentScore), 320.0f, this->Height / 2.0f +40.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        Text->RenderText("Press ENTER to retry or ESC to quit", 130.0f, this->Height / 2.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
    }
    if(this->State == GAME_LOSS)
    {
        Text->RenderText("Player 2 WON!!!", this->Width-20.0f,  this->Height / 2.0f - 20.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        Text->RenderText("Score: " + std::to_string(playerScore)+" - " + std::to_string(opponentScore), 320.0f, this->Height / 2.0f +40.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        Text->RenderText("Press ENTER to retry or ESC to quit", 130.0f, this->Height / 2.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
    }
}

void Game::ResetGame()
{
    Player->Position = glm::vec2(0, this->Height /2.0f  - PLAYER_SIZE.y/2);
    Opponent->Position = glm::vec2(this->Width-PLAYER_SIZE.x, this->Height/2.0f - PLAYER_SIZE.y/2);
    

   std::srand(std::time(nullptr));

    // Constants
    const float PI = 3.14159265f;
    const float strength = 1000.0f;

    // Generate a random angle between 0 and 2*PI
    float randomAngle = (std::rand() % 360) * PI / 180.0f; 

    // Calculate velocity components using trigonometry
    float randomX = strength * cos(randomAngle); 
    float randomY = strength * sin(randomAngle);

    // Create the velocity vector
    ballVelocity = glm::vec2(randomX + 100.0f, randomY + 100.0f);
    Ball->Reset(glm::vec2(this->Width/2, this->Height/2), ballVelocity);
    isUpdating = false;
}

bool CheckCollision(GameObject &one, GameObject &two);
Collision CheckCollision(BallObject &one, GameObject &two);
Direction VectorDirection(glm::vec2 closest);

void Game::DoCollisions()
{
    // collision resolution

    Collision collision = CheckCollision(*Ball, *Player);

    if(std::get<0>(collision))
    {
        Direction dir = std::get<1>(collision);
        glm::vec2 diff_vector = std::get<2>(collision);
        glm::vec2 oldVelocity = Ball->Velocity;
        Ball->Velocity.x = -oldVelocity.x;
        float penetration = Ball->Radius - std::abs(diff_vector.x);
        Ball->Position.x += penetration; // move ball to right
        SoundEngine->play2D("audio/bleep.wav", false);
        
    }
    Collision oppoCollision  = CheckCollision(*Ball, *Opponent);

    if(std::get<0>(oppoCollision))
    {
        Direction dir = std::get<1>(oppoCollision);
        glm::vec2 diff_vector = std::get<2>(oppoCollision);
        glm::vec2 oldVelocity = Ball->Velocity;
        Ball->Velocity.x = -oldVelocity.x;

        float penetration = Ball->Radius - std::abs(diff_vector.x);

        Ball->Position.x -= penetration; // move ball to left;
        SoundEngine->play2D("audio/bleep.wav", false);
    }
}
// collision detection

bool CheckCollision(GameObject &one, GameObject &two) // AABB - AABB collision
{
    // collision x-axis?
    bool collisionX = one.Position.x + one.Size.x > two.Position.x &&
        two.Position.x + two.Size.x > one.Position.x;
    // collision y-axis?
    bool collisionY = one.Position.y + one.Size.y > two.Position.y &&
        two.Position.y + two.Size.y > one.Position.y;
    // collision only if on both axes
    return collisionX && collisionY;
}

Collision CheckCollision(BallObject &one, GameObject &two) // AABB - Circle collision
{
    // get center point circle first 
    glm::vec2 center(one.Position + one.Radius);
    // calculate AABB info (center, half-extents)
    glm::vec2 aabb_half_extents(two.Size.x / 2.0f, two.Size.y / 2.0f);
    glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
    // get difference vector between both centers
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    // now that we know the clamped values, add this to AABB_center and we get the value of box closest to circle
    glm::vec2 closest = aabb_center + clamped;
    // now retrieve vector between center circle and closest point AABB and check if length < radius
    difference = closest - center;
    
    if (glm::length(difference) < one.Radius) // not <= since in that case a collision also occurs when object one exactly touches object two, which they are at the end of each collision resolution stage.
        return std::make_tuple(true, VectorDirection(difference), difference);
    else
        return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
}

// calculates which direction a vector is facing (N,E,S or W)
Direction VectorDirection(glm::vec2 target)
{
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),	// up
        glm::vec2(1.0f, 0.0f),	// right
        glm::vec2(0.0f, -1.0f),	// down
        glm::vec2(-1.0f, 0.0f)	// left
    };
    float max = 0.0f;
    unsigned int best_match = -1;
    for (unsigned int i = 0; i < 4; i++)
    {
        float dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max)
        {
            max = dot_product;
            best_match = i;
        }
    }
    return (Direction)best_match;
}
