# Multi Game

## Overview
Multi Game is a desktop application developed in C++ using the **Raylib** graphics library.  
The project integrates multiple classic games into a single platform with a user authentication system and persistent score tracking.  
The objective of this project is to apply object-oriented programming concepts, file handling, and graphical event management in a practical and structured manner.  

The application allows users to create an account, log in securely, play games, and maintain individual high scores across sessions.

## Features
- User login and sign-up system
- Persistent storage of user credentials and game scores
- Three integrated games within a single interface
- Graphical user interface using Raylib
- Restart and navigation controls for each game
- Modular and object-oriented design

## Games Implemented

### Tic Tac Toe
A two-player turn-based game implemented on a 3×3 grid. The game detects winning conditions, draws, and updates scores accordingly.

### Memory Match
A card-matching game using a 4×4 grid. Card values are randomized at each restart. The game uses timed card reveals and score updates based on successful matches.

### Hangman
A word-guessing game where the player must identify a hidden word within a limited number of incorrect attempts.  
The game includes a visual representation of the hangman structure and displays the result upon completion.

## Concepts Applied
- Object-Oriented Programming
- File handling for persistent data storage
- Event handling using keyboard and mouse input
- Game state management
- Use of STL containers such as vectors and strings
- Randomization and timing logic

## Technologies Used
- Programming Language: C++
- Graphics Library: Raylib
- Compiler: GCC / MinGW
- Development Environment: Visual Studio Code

## How to Run

### Requirements
- Raylib installed and configured
- GCC compiler available in system path

### Compilation
```bash
g++ game.cpp -o game -lraylib -lopengl32 -lgdi32 -lwinmm