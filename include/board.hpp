#ifndef PROJECT_BASE_BOARD_HPP
#define PROJECT_BASE_BOARD_HPP

#include <glm/glm.hpp>

#include <vector>
#include <string>

class Board {
public:
    vector <vector<string>> board;
    glm::vec3 get_position(int row, char col);
    string get_piece(int row, char col);
    void set_piece(int row, char col, string piece);
    Board();

private:

};

Board::Board() {
    board.resize(8);
    for(int i = 0; i < 8; i++) {
        board[i] = vector<string>(8, "");
    }
    set_piece(1, 'a', "rook_white");
    set_piece(1, 'b', "knight_white");
    set_piece(1, 'c', "bishop_white");
    set_piece(1, 'd', "queen_white");
    set_piece(1, 'e', "king_white");
    set_piece(1, 'f', "bishop_white");
    set_piece(1, 'g', "knight_white");
    set_piece(1, 'h', "rook_white");
    for(int i = 0; i < 8; i++) {
        set_piece(2, (char)((int)'a'+i), "pawn_white");
    }
    set_piece(8, 'a', "rook_black");
    set_piece(8, 'b', "knight_black");
    set_piece(8, 'c', "bishop_black");
    set_piece(8, 'd', "queen_black");
    set_piece(8, 'e', "king_black");
    set_piece(8, 'f', "bishop_black");
    set_piece(8, 'g', "knight_black");
    set_piece(8, 'h', "rook_black");
    for(int i = 0; i < 8; i++) {
        set_piece(7, (char)((int)'a'+i), "pawn_black");
    }
}

glm::vec3 Board::get_position(int row, char col) {
    col -= 'a';
    row -= 1;
    return {-3.5f+(float)col,-3.5f+(float)row,0.0f};
}

string Board::get_piece(int row, char col) {
    col -= 'a';
    row -= 1;
    return board[row][col];
}

void Board::set_piece(int row, char col, string piece) {
    col -= 'a';
    row -= 1;
    board[row][col] = piece;
}

#endif //PROJECT_BASE_BOARD_HPP
