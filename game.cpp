#include "raylib.h"
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <random>

struct User {
    std::string username;
    std::string password;
    int tictactoeHighScore = 0;
    int memoryMatchHighScore = 0;
    int hangmanHighScore = 0;
};

std::vector<User> users;
User* currentUser = nullptr;

void loadUsers() {
    std::ifstream file("users.txt");
    if (!file) return;
    User u;
    while (file >> u.username >> u.password >> u.tictactoeHighScore >> u.memoryMatchHighScore >> u.hangmanHighScore)
        users.push_back(u);
}

void saveUsers() {
    std::ofstream file("users.txt");
    for (auto& u : users)
        file << u.username << " " << u.password << " " << u.tictactoeHighScore << " "
             << u.memoryMatchHighScore << " " << u.hangmanHighScore << "\n";
}

User* findUser(std::string uname, std::string pass) {
    for (auto& u : users)
        if (u.username == uname && u.password == pass)
            return &u;
    return nullptr;
}

User* createUser(std::string uname, std::string pass) {
    users.push_back({uname, pass});
    return &users.back();
}

// ---------- Tic Tac Toe --------------
class GameTicTacToe {
    char board[3][3];
    char currentPlayer;
    bool gameOver;
    char winner;
public:
    int score;

    GameTicTacToe() { reset(); }

    void reset() {
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                board[i][j] = ' ';
        currentPlayer = 'X';
        gameOver = false;
        winner = ' ';
        score = 0;
    }

    void update(int row, int col) {
        if (gameOver || board[row][col] != ' ') return;
        board[row][col] = currentPlayer;
        checkWinner();
        if (!gameOver) currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    }

    void draw() {
        int cellSize = 100;
        int offsetX = GetScreenWidth()/2 - (cellSize * 3)/2;
        int offsetY = 100;
        for (int i = 0; i < 4; i++) {
            DrawLine(offsetX, offsetY + i * cellSize, offsetX + 3 * cellSize, offsetY + i * cellSize, BLACK);
            DrawLine(offsetX + i * cellSize, offsetY, offsetX + i * cellSize, offsetY + 3 * cellSize, BLACK);
        }

        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                int x = offsetX + j * cellSize + 30;
                int y = offsetY + i * cellSize + 30;
                if (board[i][j] == 'X')
                    DrawText("X", x, y, 40, RED);
                else if (board[i][j] == 'O')
                    DrawText("O", x, y, 40, BLUE);
            }

        if (gameOver) {
            std::string msg = (winner == ' ') ? "Draw!" : std::string("Winner: ") + winner;
            DrawText(msg.c_str(), GetScreenWidth()/2 - MeasureText(msg.c_str(), 40)/2, 450, 40, DARKGREEN);
        } else {
            std::string turn = "Turn: ";
            turn += currentPlayer;
            DrawText(turn.c_str(), GetScreenWidth()/2 - MeasureText(turn.c_str(), 30)/2, 50, 30, DARKGRAY);
        }
    }

    bool isGameOver() { return gameOver; }
    char getWinner() { return winner; }
    char getCurrentPlayer() { return currentPlayer; }

    void checkWinner() {
        for (int i = 0; i < 3; ++i) {
            if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
                winner = board[i][0]; gameOver = true; score = 1; return;
            }
            if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i]) {
                winner = board[0][i]; gameOver = true; score = 1; return;
            }
        }

        if (board[0][0] != ' ' && board[0][0]==board[1][1] && board[1][1]==board[2][2]) {
            winner = board[0][0]; gameOver = true; score = 1; return;
        }
        if (board[0][2] != ' ' && board[0][2]==board[1][1] && board[1][1]==board[2][0]) {
            winner = board[0][2]; gameOver = true; score = 1; return;
        }

        bool draw = true;
        for (int i = 0; i < 3 && draw; ++i)
            for (int j = 0; j < 3 && draw; ++j)
                if (board[i][j] == ' ') draw = false;

        if (draw) {
            winner = ' ';
            gameOver = true;
            score = 0;
        }
    }

    bool handleInput() {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int cellSize = 100;
            int offsetX = GetScreenWidth()/2 - (cellSize * 3)/2;
            int offsetY = 100;
            int x = GetMouseX() - offsetX;
            int y = GetMouseY() - offsetY;
            int col = x / cellSize;
            int row = y / cellSize;
            if (row >= 0 && row < 3 && col >= 0 && col < 3)
                update(row, col);
        }
        return IsKeyPressed(KEY_R);
    }
};

// ---------- Memory Match Game --------------
class MemoryCard {
public:
    Rectangle rect;
    int value;
    bool revealed = false;
    bool matched = false;

    MemoryCard(float x, float y, float size, int val) : value(val) {
        rect = {x, y, size, size};
    }

    void draw() {
        if (matched) {
            DrawRectangleRec(rect, DARKGREEN);
        } else if (revealed) {
            DrawRectangleRec(rect, LIGHTGRAY);
            DrawText(std::to_string(value).c_str(),
                     rect.x + rect.width / 2 - 10,
                     rect.y + rect.height / 2 - 10,
                     20, BLACK);
        } else {
            DrawRectangleRec(rect, GRAY);
        }
        DrawRectangleLinesEx(rect, 2, DARKGRAY);
    }

    bool isClicked(Vector2 mouse) {
        return CheckCollisionPointRec(mouse, rect) && !revealed && !matched;
    }
};

class GameMemoryMatch {
    std::vector<MemoryCard> cards;
    int gridSize = 4;
    float cardSize = 100;
    MemoryCard* first = nullptr;
    MemoryCard* second = nullptr;
    double revealStartTime = 0;
    bool waiting = false;
public:
    int score = 0;

    GameMemoryMatch(int screenWidth, int screenHeight) {
        initCards(screenWidth, screenHeight);
    }

    void initCards(int screenWidth, int screenHeight) {
        cards.clear();
        std::vector<int> values;
        for (int i = 0; i < (gridSize * gridSize) / 2; i++) {
            values.push_back(i);
            values.push_back(i);
        }
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(values.begin(), values.end(), g);

        float offsetX = (screenWidth - gridSize * cardSize) / 2;
        float offsetY = (screenHeight - gridSize * cardSize) / 2;

        for (int row = 0; row < gridSize; row++) {
            for (int col = 0; col < gridSize; col++) {
                float x = offsetX + col * cardSize;
                float y = offsetY + row * cardSize;
                int idx = row * gridSize + col;
                cards.push_back(MemoryCard(x, y, cardSize - 10, values[idx]));
            }
        }
        first = nullptr;
        second = nullptr;
        waiting = false;
        score = 0;
    }

    void update() {
        Vector2 mouse = GetMousePosition();

        if (!waiting && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (auto& card : cards) {
                if (card.isClicked(mouse)) {
                    card.revealed = true;
                    if (!first)
                        first = &card;
                    else if (!second && &card != first)
                        second = &card;
                    break;
                }
            }
            if (first && second) {
                waiting = true;
                revealStartTime = GetTime();
            }
        }

        if (waiting && GetTime() - revealStartTime > 1.0) {
            if (first->value == second->value) {
                first->matched = true;
                second->matched = true;
                score++;
            } else {
                first->revealed = false;
                second->revealed = false;
            }
            first = nullptr;
            second = nullptr;
            waiting = false;
        }
    }

    void draw() {
        for (auto& card : cards)
            card.draw();
        if (isGameOver()) {
            const char* winText="You win.";
            int fontSize=40;
            int textWidth=MeasureText(winText, fontSize);
            DrawText(winText, GetScreenWidth()/2-textWidth/2,30,fontSize,DARKGREEN);
        }
    }

    bool isGameOver() {
        for (auto& card : cards)
            if (!card.matched) return false;
        return true;
    }

    bool handleInput() {
        return IsKeyPressed(KEY_R);
    }
};

// -------- Hangman Game --------------
class GameHangman {
    std::string word;
    std::vector<bool> guessed;
    std::vector<char> wrongLetters;
    int maxWrong;
    int wrongCount;
    bool gameOver;
    bool won;

    std::vector<std::string> wordList = {
        "computer", "program", "engineer", "hangman", "keyboard",
        "variable", "function", "object", "class", "pointer"
    };

public:
    int score = 0;

    GameHangman() {
        reset();
    }

    void reset() {
        srand(time(0));
        word = wordList[rand() % wordList.size()];
        guessed = std::vector<bool>(word.size(), false);
        wrongLetters.clear();
        maxWrong = 6;
        wrongCount = 0;
        gameOver = false;
        won = false;
        score = 0;
    }

    void update() {
        if (gameOver) return;

        for (int key = KEY_A; key <= KEY_Z; key++) {
            if (IsKeyPressed(key)) {
                char guess = (char)(key - KEY_A + 'a');
                if (isAlreadyGuessed(guess))
                    return;

                bool found = false;
                for (int i = 0; i < (int)word.size(); i++) {
                    if (word[i] == guess) {
                        guessed[i] = true;
                        found = true;
                    }
                }

                if (!found) {
                    wrongLetters.push_back(guess);
                    wrongCount++;
                }

                won = true;
                for (bool b : guessed) {
                    if (!b) {
                        won = false;
                        break;
                    }
                }

                if (wrongCount >= maxWrong || won) {
                    gameOver = true;
                    if (won) score = 1;
                }

                break;
            }
        }
    }

    void draw() {
        DrawText("Hangman Game", 220, 20, 30, DARKGRAY);

        std::string display = "";
        for (int i = 0; i < (int)word.size(); i++) {
            if (guessed[i])
                display += word[i];
            else
                display += '_';
            display += ' ';
        }
        DrawText(display.c_str(), 180, 100, 40, BLACK);

        std::string wrongStr = "Wrong guesses: ";
        for (char c : wrongLetters)
            wrongStr += c, wrongStr += ' ';
        DrawText(wrongStr.c_str(), 150, 200, 20, RED);

        int baseX = 60;
        int baseY = 350;

        DrawLine(baseX, baseY, baseX + 120, baseY, BLACK);
        DrawLine(baseX + 30, baseY, baseX + 30, baseY - 250, BLACK);
        DrawLine(baseX + 30, baseY - 250, baseX + 90, baseY - 250, BLACK);
        DrawLine(baseX + 90, baseY - 250, baseX + 90, baseY - 220, BLACK);

        if (wrongCount > 0) DrawCircle(baseX + 90, baseY - 200, 20, BLACK);
        if (wrongCount > 1) DrawLine(baseX + 90, baseY - 180, baseX + 90, baseY - 100, BLACK);
        if (wrongCount > 2) DrawLine(baseX + 90, baseY - 170, baseX + 60, baseY - 140, BLACK);
        if (wrongCount > 3) DrawLine(baseX + 90, baseY - 170, baseX + 120, baseY - 140, BLACK);
        if (wrongCount > 4) DrawLine(baseX + 90, baseY - 100, baseX + 60, baseY - 60, BLACK);
        if (wrongCount > 5) DrawLine(baseX + 90, baseY - 100, baseX + 120, baseY - 60, BLACK);

        if (gameOver) {
            if (won) {
                DrawText("ðŸŽ‰ You Win!", 180, 320, 30, DARKGREEN);
            } else {
                DrawText("ðŸ˜­ You Lose! Word was:", 160, 320, 25, RED);
                DrawText(word.c_str(), 260, 360, 40, RED);
            }
        } else {
            DrawText("Guess a letter (A-Z)", 200, 270, 20, DARKGRAY);
        }
    }

    bool isGameOver() { return gameOver; }
    bool handleInput() { return IsKeyPressed(KEY_R); }

private:
    bool isAlreadyGuessed(char c) {
        for (int i = 0; i < (int)word.size(); i++)
            if (guessed[i] && word[i] == c)
                return true;
        for (char wc : wrongLetters)
            if (wc == c)
                return true;
        return false;
    }
};


// ---------- Main --------------

enum ScreenState {
    LOGIN_SCREEN,
    SIGNUP_SCREEN,
    HOME_SCREEN,
    GAME_TICTACTOE,
    GAME_MEMORYMATCH,
    GAME_HANGMAN
};

int main() {
    const int screenWidth = 600;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Multi Game Hub");
    SetTargetFPS(60);

    loadUsers();

    ScreenState screen = LOGIN_SCREEN;

    std::string inputUsername = "";
    std::string inputPassword = "";
    bool passwordField = false; // false = username field selected, true = password field selected

    bool signinChosen = false; // false means login selected; true means sign up selected

    GameTicTacToe ticTacToe;
    GameMemoryMatch memoryMatch(screenWidth, screenHeight);
    GameHangman hangman;

    bool gameRunning = false;
    bool gameRestartRequested = false;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // ---------- LOGIN or SIGNUP Screen --------------
        if (screen == LOGIN_SCREEN || screen == SIGNUP_SCREEN) {
            // Title
            std::string title = (screen == LOGIN_SCREEN) ? "Login" : "Sign Up";
            DrawText(title.c_str(), screenWidth/2 - MeasureText(title.c_str(), 40)/2, 50, 40, DARKGRAY);

            // Input labels
            DrawText("Username:", 100, 150, 20, BLACK);
            DrawText("Password:", 100, 220, 20, BLACK);

            // Boxes
            Rectangle userBox = {250, 145, 250, 30};
            Rectangle passBox = {250, 215, 250, 30};

            // Draw input box backgrounds
            DrawRectangleRec(userBox, (passwordField == false) ? LIGHTGRAY : GRAY);
            DrawRectangleRec(passBox, (passwordField == true) ? LIGHTGRAY : GRAY);

            // Draw input texts
            DrawText(inputUsername.c_str(), 255, 150, 20, BLACK);

            // Mask password input
            std::string maskedPass(inputPassword.size(), '*');
            DrawText(maskedPass.c_str(), 255, 220, 20, BLACK);

            // Instructions
            std::string inst = "Press TAB to switch field. Press ENTER to submit.";
            DrawText(inst.c_str(), screenWidth/2 - MeasureText(inst.c_str(), 20)/2, 280, 20, DARKGRAY);

            // Toggle between login and signup
            std::string toggleText = (screen == LOGIN_SCREEN) ? "Press S to Sign Up" : "Press L to Login";
            DrawText(toggleText.c_str(), screenWidth/2 - MeasureText(toggleText.c_str(), 20)/2, 320, 20, BLUE);

            // Input handling
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 126)) { // Printable chars
                    if (passwordField) {
                        inputPassword.push_back((char)key);
                    } else {
                        inputUsername.push_back((char)key);
                    }
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE)) {
                if (passwordField && !inputPassword.empty())
                    inputPassword.pop_back();
                else if (!passwordField && !inputUsername.empty())
                    inputUsername.pop_back();
            }

            if (IsKeyPressed(KEY_TAB)) {
                passwordField = !passwordField;
            }

            if (IsKeyPressed(KEY_S)) {
                screen = SIGNUP_SCREEN;
                inputUsername.clear();
                inputPassword.clear();
                passwordField = false;
            }

            if (IsKeyPressed(KEY_L)) {
                screen = LOGIN_SCREEN;
                inputUsername.clear();
                inputPassword.clear();
                passwordField = false;
            }

            if (IsKeyPressed(KEY_ENTER)) {
                if (screen == LOGIN_SCREEN) {
                    User* u = findUser(inputUsername, inputPassword);
                    if (u != nullptr) {
                        currentUser = u;
                        screen = HOME_SCREEN;
                        inputUsername.clear();
                        inputPassword.clear();
                        passwordField = false;
                    }
                } else {
                    // Sign up
                    if (!inputUsername.empty() && !inputPassword.empty()) {
                        bool exists = false;
                        for (auto& u : users) {
                            if (u.username == inputUsername) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            currentUser = createUser(inputUsername, inputPassword);
                            saveUsers();
                            screen = HOME_SCREEN;
                            inputUsername.clear();
                            inputPassword.clear();
                            passwordField = false;
                        }
                    }
                }
            }
        }

        // -------- HOME SCREEN --------------
        else if (screen == HOME_SCREEN) {
            std::string welcome = "Welcome, " + currentUser->username;
            DrawText(welcome.c_str(), screenWidth/2 - MeasureText(welcome.c_str(), 30)/2, 40, 30, DARKGREEN);

            int btnWidth = 300;
            int btnHeight = 50;
            int centerX = screenWidth/2 - btnWidth/2;
            int startY = 120;
            int gap = 80;

            Rectangle btnTicTac = { (float)centerX, (float)startY, (float)btnWidth, (float)btnHeight };
            Rectangle btnMemory = { (float)centerX, (float)(startY + gap), (float)btnWidth, (float)btnHeight };
            Rectangle btnHangman = { (float)centerX, (float)(startY + 2*gap), (float)btnWidth, (float)btnHeight };
            Rectangle btnLogout = { (float)centerX, (float)(startY + 3 * gap), (float)btnWidth, (float)btnHeight };
            
            DrawRectangleRec(btnTicTac, LIGHTGRAY);
            DrawRectangleLinesEx(btnTicTac, 2, BLACK);
            DrawText("Play Tic Tac Toe", centerX + 60, startY + 15, 20, BLACK);

            DrawRectangleRec(btnMemory, LIGHTGRAY);
            DrawRectangleLinesEx(btnMemory, 2, BLACK);
            DrawText("Play Memory Match", centerX + 50, startY + gap + 15, 20, BLACK);

            DrawRectangleRec(btnHangman, LIGHTGRAY);
            DrawRectangleLinesEx(btnHangman, 2, BLACK);
            DrawText("Play Hangman", centerX + 90, startY + 2*gap + 15, 20, BLACK);

            DrawRectangleRec(btnLogout, LIGHTGRAY);
            DrawRectangleLinesEx(btnLogout, 2, BLACK);
            DrawText("Log Out", centerX + 100, startY + 3 * gap + 15, 20, RED);

            // Display High Scores
            std::string scoresText = "High Scores:";
            DrawText(scoresText.c_str(), 20, screenHeight - 130, 20, DARKGRAY);
            DrawText(("Tic Tac Toe: " + std::to_string(currentUser->tictactoeHighScore)).c_str(), 20, screenHeight - 100, 20, DARKGRAY);
            DrawText(("Memory Match: " + std::to_string(currentUser->memoryMatchHighScore)).c_str(), 20, screenHeight - 75, 20, DARKGRAY);
            DrawText(("Hangman: " + std::to_string(currentUser->hangmanHighScore)).c_str(), 20, screenHeight - 50, 20, DARKGRAY);

            if (CheckCollisionPointRec(GetMousePosition(), btnTicTac) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                ticTacToe.reset();
                screen = GAME_TICTACTOE;
            }
            if (CheckCollisionPointRec(GetMousePosition(), btnMemory) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                memoryMatch.initCards(screenWidth, screenHeight);
                screen = GAME_MEMORYMATCH;
            }
            if (CheckCollisionPointRec(GetMousePosition(), btnHangman) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                hangman.reset();
                screen = GAME_HANGMAN;
            }

             if (CheckCollisionPointRec(GetMousePosition(), btnLogout) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentUser = nullptr;
                inputUsername.clear();
                inputPassword.clear();
                passwordField = false;
                screen = LOGIN_SCREEN;
        }

        }

        // -------- Tic Tac Toe Game ----------
        else if (screen == GAME_TICTACTOE) {
            ticTacToe.draw();
            ticTacToe.handleInput();
            if (!ticTacToe.isGameOver()) {
                ticTacToe.handleInput();
            }
            if (ticTacToe.isGameOver()) {
                if (ticTacToe.getWinner() == currentUser->username[0]) {}
                if (ticTacToe.score > currentUser->tictactoeHighScore)
                    currentUser->tictactoeHighScore = ticTacToe.score;
                saveUsers();
            }
            // Buttons
            int btnWidth = 100;
            int btnHeight = 40;
            int yPos = 520;
            Rectangle btnRestart = {(float)(screenWidth/4 - btnWidth/2), (float)yPos, (float)btnWidth, (float)btnHeight};
            Rectangle btnHome = {(float)(3*screenWidth/4 - btnWidth/2), (float)yPos, (float)btnWidth, (float)btnHeight};

            DrawRectangleRec(btnRestart, LIGHTGRAY);
            DrawRectangleLinesEx(btnRestart, 2, BLACK);
            DrawText("Restart", btnRestart.x + 20, btnRestart.y + 10, 20, BLACK);

            DrawRectangleRec(btnHome, LIGHTGRAY);
            DrawRectangleLinesEx(btnHome, 2, BLACK);
            DrawText("Home", btnHome.x + 30, btnHome.y + 10, 20, BLACK);

            if (CheckCollisionPointRec(GetMousePosition(), btnRestart) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                ticTacToe.reset();
            }
            if (CheckCollisionPointRec(GetMousePosition(), btnHome) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                screen = HOME_SCREEN;
            }
        }

        // -------- Memory Match Game ----------
        else if (screen == GAME_MEMORYMATCH) {
            memoryMatch.draw();
            memoryMatch.update();

            if (memoryMatch.isGameOver()) {
                if (memoryMatch.score > currentUser->memoryMatchHighScore)
                    currentUser->memoryMatchHighScore = memoryMatch.score;
                saveUsers();
            }

            // Buttons
            int btnWidth = 100;
            int btnHeight = 40;
            int yPos = 520;
            Rectangle btnRestart = {(float)(screenWidth/4 - btnWidth/2), (float)yPos, (float)btnWidth, (float)btnHeight};
            Rectangle btnHome = {(float)(3*screenWidth/4 - btnWidth/2), (float)yPos, (float)btnWidth, (float)btnHeight};

            DrawRectangleRec(btnRestart, LIGHTGRAY);
            DrawRectangleLinesEx(btnRestart, 2, BLACK);
            DrawText("Restart", btnRestart.x + 20, btnRestart.y + 10, 20, BLACK);

            DrawRectangleRec(btnHome, LIGHTGRAY);
            DrawRectangleLinesEx(btnHome, 2, BLACK);
            DrawText("Home", btnHome.x + 30, btnHome.y + 10, 20, BLACK);

            if (CheckCollisionPointRec(GetMousePosition(), btnRestart) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                memoryMatch.initCards(screenWidth, screenHeight);
            }
            if (CheckCollisionPointRec(GetMousePosition(), btnHome) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                screen = HOME_SCREEN;
            }
        }

        // -------- Hangman Game ----------
        else if (screen == GAME_HANGMAN) {
            hangman.draw();
            hangman.update();

            if (hangman.isGameOver()) {
                if (hangman.score > currentUser->hangmanHighScore)
                    currentUser->hangmanHighScore = hangman.score;
                saveUsers();
            }

            // Buttons
            int btnWidth = 100;
            int btnHeight = 40;
            int yPos = 520;
            Rectangle btnRestart = {(float)(screenWidth/4 - btnWidth/2), (float)yPos, (float)btnWidth, (float)btnHeight};
            Rectangle btnHome = {(float)(3*screenWidth/4 - btnWidth/2), (float)yPos, (float)btnWidth, (float)btnHeight};

            DrawRectangleRec(btnRestart, LIGHTGRAY);
            DrawRectangleLinesEx(btnRestart, 2, BLACK);
            DrawText("Restart", btnRestart.x + 20, btnRestart.y + 10, 20, BLACK);

            DrawRectangleRec(btnHome, LIGHTGRAY);
            DrawRectangleLinesEx(btnHome, 2, BLACK);
            DrawText("Home", btnHome.x + 30, btnHome.y + 10, 20, BLACK);

            if (CheckCollisionPointRec(GetMousePosition(), btnRestart) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                hangman.reset();
            }
            if (CheckCollisionPointRec(GetMousePosition(), btnHome) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                screen = HOME_SCREEN;
            }
        }

        EndDrawing();
    }

    CloseWindow();
    saveUsers();
    return 0;
}
