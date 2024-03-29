//SRC: J.Coppens 2019

#ifndef SWEEPERAI
#define SWEEPERAI

#include <vector>
#include "minesweeper.h"
#include "tile.h"
#include <iomanip>
#include <stdlib.h> //abs

struct Tile {
    int row;
    int col;
    int val = -1;
    bool hidden = true;
    bool isBorderNum = false;
    bool flagged = false;
    std::vector<Tile*> neighbours;
};

class MinesweeperAI {
    // Data
    public:
        int ncols;
        int nrows;
        int num_mines;
        int num_flags;
        float tilesize; 
    private:
        std::vector<std::vector<Tile>> board;
        std::vector<std::vector<Tile*>> border;
        std::vector<std::vector<int>> solutions, Mines, NoMines;
        Minesweeper *game;
        Tile default_tile;
        int unknown_tiles;

    // Methods
    public:
        MinesweeperAI (Minesweeper* gameref){
            game = gameref;
            nrows = game->nrows;
            ncols = game->ncols;
            num_mines = game->num_mines;
            tilesize = game->s;
            board = std::vector<std::vector<Tile>>(nrows, std::vector<Tile>(ncols,default_tile));
        }

        void read_board(void){
            unknown_tiles = nrows * ncols;
            for (int row=0; row<nrows; row++){
                for (int col=0; col<ncols; col++){
                    board[row][col] = default_tile;
                    board[row][col].row = row;
                    board[row][col].col = col;
                    if (game->grid[col + row * ncols].is_clicked){
                        board[row][col].val = game->grid[col + row * ncols].val;
                        board[row][col].hidden = false;
                        --unknown_tiles;
                    }
                    else if (game->grid[col + row * ncols].flagged){
                        board[row][col].val = -2;
                        board[row][col].flagged = true;
                        board[row][col].hidden = false;
                        ++num_flags;
                        --unknown_tiles;
                    }
                }
            }
            for (int i=0; i<nrows; i++){
                for (int j=0; j<ncols; j++){
                    bool oU = false, oD = false,  oL =false, oR = false;
                    oU = (i == 0);
                    oD = (i == nrows-1);
                    oL = (j == 0);
                    oR = (j == ncols-1);
                    
                    if(!oU) board[i][j].neighbours.push_back(&board[i-1][j]);
                    if(!oD) board[i][j].neighbours.push_back(&board[i+1][j]);
                    if(!oL) board[i][j].neighbours.push_back(&board[i][j-1]);
                    if(!oR) board[i][j].neighbours.push_back(&board[i][j+1]);
                    
                    if(!oU && !oL) board[i][j].neighbours.push_back(&board[i-1][j-1]);
                    if(!oU && !oR) board[i][j].neighbours.push_back(&board[i-1][j+1]);
                    if(!oD && !oL) board[i][j].neighbours.push_back(&board[i+1][j-1]);
                    if(!oD && !oR) board[i][j].neighbours.push_back(&board[i+1][j+1]); 
                }
            }

            find_border_numbers();
        }

        void compute(void){
            int passed = 0;
            for (int row=0; row<nrows; row++){
                for (int col=0; col<ncols; col++){
                    if (!board[row][col].hidden && board[row][col].isBorderNum){
                        passed += basic_hidden_neighbour_rule(row, col);
                        passed += basic_flag_rule(row, col);
                    }
                }
            }
            
            if (passed == 0){
                std::cout << "Basic tests failed\n";
                passed += tank_solver();
                std::cout << unknown_tiles << " unknown tiles (after ts)\n";
            }
            if(passed == 0){
                std::cout << "Tank solver failed\n";
            }
        }


    private:
        void click_tile(int idy, int idx){
            float x = (idx + 1.5) * tilesize;
            float y = (idy + 2.5) * tilesize;
            game->left_click(x, y);
            board[idy][idx].hidden = false;
            --unknown_tiles;
        }
        
        void flag_tile(int idy, int idx){
            float x = (idx + 1.5) * tilesize;
            float y = (idy + 2.5) * tilesize;
            if (!board[idy][idx].flagged){
                game->right_click(x, y);
                board[idy][idx].flagged = true;
                --unknown_tiles;
            }
        }

        void print_vals(void){
            for (int row=0; row<nrows; row++){
                for (int col=0; col<ncols; col++){
                    std::cout << std::setw(2) << board[row][col].val << " ";
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }

        void find_border_numbers(void){
            for (int i=0; i<nrows; i++){
                for (int j=0; j<ncols; j++){
                    if (board[i][j].flagged || board[i][j].hidden)
                        continue;
                    
                    for (auto& x : board[i][j].neighbours){
                        if(x->hidden){
                            board[i][j].isBorderNum = true;
                            break;
                        }
                    }
                }
            }
        }

        int basic_hidden_neighbour_rule(int i, int j){
            int n = 0;
            for (auto& x : board[i][j].neighbours)
                if(x->hidden || x->flagged) n++;
            
            if(n == board[i][j].val){
                for (auto& x : board[i][j].neighbours)
                    if(x->hidden) flag_tile(x->row,x->col);
                return 1;
            }
            return 0;
        }
        
        int basic_flag_rule(int i, int j){
            int n = 0;
            for (auto& x : board[i][j].neighbours)
                if(x->flagged) n++;
            
            if(n == board[i][j].val){
                for (auto& x : board[i][j].neighbours)
                    if(x->hidden) click_tile(x->row,x->col);
                return 1;
            }
            return 0;
        }
        
        int tank_solver(void){
            get_hidden_border();

            if(border.size() == 0)
                return 0;  // No border tiles

            Mines = std::vector<std::vector<int>>(nrows, std::vector<int>(ncols,0));
            NoMines = std::vector<std::vector<int>>(nrows, std::vector<int>(ncols,0));
            for (int i=0; i<nrows; i++){
                for (int j=0; j<ncols; j++){
                    if(board[i][j].flagged) Mines[i][j] = 1;
                    if(board[i][j].val >= 0) NoMines[i][j] = 1;
                }
            }

            for (int i=0; i<int(border.size()); i++){
                bool success = false;
                solutions.clear();
                tank_recurse(border[i], 0);
                
                std::vector<float> result = std::vector<float>(border[i].size(),0);
                for(auto &solution : solutions)
                    for(int j=0; j<int(solution.size()); j++)
                        result[j] += float(solution[j]);
               
                for(auto &val : result)
                    val /= solutions.size();

                //std::cout << "Result: \n"; 
                for(unsigned int j=0; j<result.size(); j++){
                    //std::cout << "(" << border[i][j]->row << "," << border[i][j]->col << ") " << result[j] << " " << std::endl;
                    if(result[j] == 0){
                        click_tile(border[i][j]->row,border[i][j]->col);
                        success = true;
                    }
                    if(result[j] == 1){
                        flag_tile(border[i][j]->row,border[i][j]->col);
                        success = true;
                    }
                }
                if(success)  return 1;
                else         return 0;

                //std::cout << std::endl;
            }
            //std::cout << "===================\n"; 

        }


        void get_hidden_border(void){
            border.clear(); 
            std::vector<Tile*> queue;
            for (int i=0; i<nrows; i++){
                for (int j=0; j<ncols; j++){
                    if (board[i][j].flagged || !board[i][j].hidden)
                        continue;
                    for (auto& x : board[i][j].neighbours){
                        if(x->hidden == false && x->flagged == false){
                            queue.push_back(&board[i][j]);
                            break;
                        }
                    }
                }
            }
            if(queue.size() == 0)
                return;

            std::cout << unknown_tiles << " unknown tiles\n";
            std::cout << queue.size() << " border tiles\n";
            std::cout << unknown_tiles - queue.size() << " outside border\n";

            border.push_back(std::vector<Tile*> (1,queue[0]));
            queue.erase(queue.begin());
            
            int i = 0;
            while(queue.begin() != queue.end()){
                unsigned int prev_size = border[i].size();
                for(auto &b : border[i]){
                    for(auto it=queue.begin(); it!=queue.end(); it++){
                        if(( abs((*it)->row - b->row) + abs((*it)->col -  b->col) ) == 1){
                            border[i].push_back(*it);
                            queue.erase(it);
                            break;
                        }
                    }
                }
                if (prev_size == border[i].size()){
                    border.push_back(std::vector<Tile*> (1,queue[0]));
                    queue.erase(queue.begin());
                    i++;
                }
            }
        }

        void tank_recurse(std::vector<Tile*> border_region, unsigned int k){
            int flag_count = 0;
            // Check all tiles to see if current mine and empty tile arrangement makes sense
            for (int i=0; i<nrows; i++){
                for (int j=0; j<ncols; j++){
                    if(Mines[i][j])
                        flag_count++;

                    if (board[i][j].hidden || board[i][j].flagged)
                        continue;
                    
                    int mine_count = 0;
                    int free_count = 0;
                    for (auto& x : board[i][j].neighbours){
                        if(Mines[x->row][x->col]) mine_count++;
                        if(NoMines[x->row][x->col]) free_count++;
                    }
                    if (mine_count > board[i][j].val)
                        return; //solution fails: too many mines
                    
                    int num_neighbours = board[i][j].neighbours.size(); 
                    if (num_neighbours - free_count < board[i][j].val)
                        return; // solution fails: too many free tiles(non-mines)
                }
            }

            if (flag_count > num_mines)
                return; // solution fails: too many mines

            if (k == border_region.size()){
                /* if(flag_count < num_mines){ */
                /*     std::cout << "Failed rule #4\n"; */
                /*     return; //solution fails: not enough mines */
                /* } */

                // Add to solutions
                std::vector<int> solution; //placeholder
                for(auto &x : border_region)
                    solution.push_back(Mines[x->row][x->col]);
                solutions.push_back(solution);

                return;
            }

            int idx = border_region[k]->col;
            int idy = border_region[k]->row;

            Mines[idy][idx] = true;
            tank_recurse(border_region, k+1);
            Mines[idy][idx] = false;
            
            NoMines[idy][idx] = true;
            tank_recurse(border_region, k+1);
            NoMines[idy][idx] = false;

        }

};

#endif
