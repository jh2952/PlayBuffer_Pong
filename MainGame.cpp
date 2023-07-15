// TO-DO:
// - Add Obstacles
// - Add Left-Right Movement
//   - Draw Boundary Lines to show player territory
// - Particle Effect from player on scoring
// - Ball flash-in before reserving

#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#define VK_KEY_W 0x57
#define VK_KEY_S 0x53

#include "Play.h"

const int DISPLAY_WIDTH = 1280;
const int DISPLAY_HEIGHT = 720;
const int DISPLAY_SCALE = 1;

enum GameObjectType
{
	TYPE_NULL = -1,
	TYPE_PLAYER1,
	TYPE_PLAYER2,
	TYPE_BALL,
	TYPE_OBSTACLE,
};

struct GameState
{
	int p1_score{};
	int p2_score{};
};
GameState gameState{};

// Forward Declerations
void UpdateBall();
void UpdatePlayers();
void HandleControls(GameObject& obj, int up, int down);
bool RectangularCollision(GameObject& obj_1, GameObject& obj_2);

// The entry point for a PlayBuffer program
void MainGameEntry(PLAY_IGNORE_COMMAND_LINE)
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	Play::CentreAllSpriteOrigins();
	Play::CreateGameObject(TYPE_PLAYER1, { DISPLAY_WIDTH / 10, DISPLAY_HEIGHT / 2 }, 0, "player_off");
	Play::CreateGameObject(TYPE_PLAYER2, { DISPLAY_WIDTH - 128, DISPLAY_HEIGHT / 2 }, 0, "player_off");
	Play::CreateGameObject(TYPE_BALL, { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, 10, "ball");
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	Play::ClearDrawingBuffer(Play::cBlack);
	Play::DrawLine({ DISPLAY_WIDTH / 2, 0 }, { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT }, Play::cWhite);
	UpdatePlayers();
	UpdateBall();
	Play::DrawFontText("132px", std::to_string(gameState.p1_score) + " " + std::to_string(gameState.p2_score),
		{ DISPLAY_WIDTH / 2, 50 }, Play::CENTRE);
	Play::PresentDrawingBuffer();
	return Play::KeyDown(VK_ESCAPE);
}

// Gets called once when the player quits the game 
int MainGameExit(void)
{
	Play::DestroyManager();
	return PLAY_OK;
}

void UpdatePlayers()
{
	// Create References to players
	GameObject& obj_player1{ Play::GetGameObjectByType(TYPE_PLAYER1) };
	GameObject& obj_player2{ Play::GetGameObjectByType(TYPE_PLAYER2) };

	// Set player keybinds and handle their controls
	HandleControls(obj_player1, VK_KEY_W, VK_KEY_S);
	HandleControls(obj_player2, VK_UP, VK_DOWN);

	// Send update to screen
	Play::UpdateGameObject(obj_player1);
	Play::UpdateGameObject(obj_player2);

	// Check that player is within display bounds
	if (Play::IsLeavingDisplayArea(obj_player1)) {
		obj_player1.pos = obj_player1.oldPos;
	}
	if (Play::IsLeavingDisplayArea(obj_player2)) {
		obj_player2.pos = obj_player2.oldPos;
	}

	// Draw Sprites
	Play::DrawSprite(obj_player1.spriteId, obj_player1.pos, 0);
	Play::DrawSprite(obj_player2.spriteId, obj_player2.pos, 0);
}

void UpdateBall()
{
	GameObject& obj_ball{ Play::GetGameObjectByType(TYPE_BALL) };
	GameObject& obj_player1{ Play::GetGameObjectByType(TYPE_PLAYER1) };
	GameObject& obj_player2{ Play::GetGameObjectByType(TYPE_PLAYER2) };

	// Serve the ball
	if (obj_ball.velocity.x == 0)
		obj_ball.velocity.x = 10 * Play::RandomRollRange(-1, 1);

	if (RectangularCollision(obj_player1, obj_ball))
	{
		// Introduce random vertical velocity following intial serve	
		if (obj_ball.velocity.y == 0)
			obj_ball.velocity.y = Play::RandomRollRange(-1, 1);

		// Correct for tunneling error in collision detection
		obj_ball.pos = obj_ball.oldPos;

		// Return the ball with players vertical momentum
		obj_ball.velocity.x *= -1;
		obj_ball.velocity.y += obj_player1.velocity.y / 2;
		Play::PlayAudio("hit");
	}
	else if (RectangularCollision(obj_player2, obj_ball))
	{
		// Introduce random vertical velocity following intial serve
		if (obj_ball.velocity.y == 0)
			obj_ball.velocity.y = Play::RandomRollRange(-1, 1);

		// Correct for tunneling error in collision detection
		obj_ball.pos = obj_ball.oldPos;

		// Return the ball with players vertical momentum
		obj_ball.velocity.x *= -1;
		obj_ball.velocity.y += obj_player2.velocity.y / 2;
		Play::PlayAudio("hit");
	}

	// Increase p1 score
	if (obj_ball.pos.x <= DISPLAY_WIDTH / 10)
	{
		gameState.p2_score += 1;
		obj_ball.velocity *= 0;
		obj_ball.pos = { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 };
		Play::PlayAudio("score");
	}
	// Increase p2 score
	else if (obj_ball.pos.x >= DISPLAY_WIDTH - 128)
	{
		gameState.p1_score += 1;
		obj_ball.velocity *= 0;
		obj_ball.pos = { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 };
		Play::PlayAudio("score");
	}

	Play::UpdateGameObject(obj_ball);

	if (Play::IsLeavingDisplayArea(obj_ball))
	{
		obj_ball.pos = obj_ball.oldPos;
		obj_ball.velocity.y *= -1;
		Play::PlayAudio("wall");
	}

	Play::DrawSprite(obj_ball.spriteId, obj_ball.pos, 0);
}

void HandleControls(GameObject& obj, int up, int down)
{
	int speed = 15;

	if (Play::KeyDown(up))
	{
		obj.velocity.y = -speed;
	}
	else if (Play::KeyDown(down))
	{
		obj.velocity.y = speed;
	}
	else
	{
		obj.velocity *= 0.7f;
	}
}

// Improved collision algorithm to prevent bugs from circular collision detection
bool RectangularCollision(GameObject& obj_1, GameObject& obj_2)
{
	int halfWidth1 = Play::GetSpriteWidth(obj_1.spriteId) / 2;
	int halfWidth2 = Play::GetSpriteWidth(obj_2.spriteId) / 2;
	int halfHeight1 = Play::GetSpriteHeight(obj_1.spriteId) / 2;
	int halfHeight2 = Play::GetSpriteHeight(obj_2.spriteId) / 2;
	
	// Check if x-axes are overlapping
	bool xOverlap = (obj_1.pos.x + halfWidth1 >= obj_2.pos.x - halfWidth2) && (obj_2.pos.x + halfWidth2 >= obj_1.pos.x - halfWidth1);
	// Check if y-axes are overlapping
	bool yOverlap = (obj_1.pos.y + halfHeight1 >= obj_2.pos.y - halfHeight2) && (obj_2.pos.y + halfHeight1 >= obj_1.pos.y - halfHeight2);

	return xOverlap && yOverlap;
}