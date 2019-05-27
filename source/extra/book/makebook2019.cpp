#include "../../types.h"

#if defined (ENABLE_MAKEBOOK_CMD)

#include "book.h"
#include "../../position.h"
#include "../../thread.h"
#include <fstream>
#include <sstream>
#include <unordered_set>

using namespace std;
using namespace Book;

namespace Book { extern void makebook_cmd(Position& pos, istringstream& is); }

namespace {

	// ----------------------------
	// �e���V���b�N��Ղ̐���
	// ----------------------------

	// cf.
	// �e���V���b�N��Ղ̐�����@
	// http://yaneuraou.yaneu.com/2019/04/19/%E3%83%86%E3%83%A9%E3%82%B7%E3%83%A7%E3%83%83%E3%82%AF%E5%AE%9A%E8%B7%A1%E3%81%AE%E7%94%9F%E6%88%90%E6%89%8B%E6%B3%95/

	// build_tree_nega_max()�ŗp����Ԃ��l�ɗp����B
	// ����̕]���l�A�w����Aleaf node�܂ł̎萔
	struct VMD
	{
		VMD() : value(-VALUE_INFINITE), move(MOVE_NONE), depth(DEPTH_ZERO) {}
		VMD(Value value_, Move move_, Depth depth_) : value(value_), move(move_), depth(depth_) {}

		Value value; // �]���l
		Move move;   // ����
		Depth depth; // �����leaf node�܂ł̎萔
	};

	// build_tree_nega_max()�ŗp����Ԃ��l�ɗp����B
	// root_color��BLACK�p��white�p�ƂŌʂ�VMD���i�[���Ă���B
	// root_color�Ƃ����̂�NegaMax����Ƃ��̌��݂�node��color���ƍl���Ė��Ȃ��B
	struct VMD_Pair
	{
		// �������������Ȃ����AVMD�N���X���̋K��̃R���X�g���N�^�ŏ������͂���Ă���B
		VMD_Pair() {}

		// black��white�Ƃ𓯂��l�ŏ���������B
		VMD_Pair(Value v, Move m, Depth d) : black(v, m, d), white(v, m, d) {}

		// black,white�����ꂼ��̒l�ŏ���������B
		VMD_Pair(Value black_v, Move black_m, Depth black_d, Value white_v, Move white_m, Depth white_d) :
			black(black_v, black_m, black_d), white(white_v, white_m, white_d) {}
		VMD_Pair(VMD black_, VMD white_) : black(black_), white(white_) {}
		VMD_Pair(VMD best[COLOR_NB]) : black(best[BLACK]), white(best[WHITE]) {}

		VMD black; // root_color == BLACK�p�̕]���l
		VMD white; // root_color == WHITE�p�̕]���l
	};

	// ��Ղ�builder
	struct BookTreeBuilder
	{
		// ���game tree�𐶐�����@�\
		void build_tree(Position& pos, istringstream& is);

		// ��Ճt�@�C����ǂݍ���ŁA�w��ǖʂ���[�@�肷�邽�߂ɕK�v�Ȋ����𐶐�����B
		void extend_tree(Position& pos, istringstream& is);

		// ��Ղ̖��������@��
		void endless_extend_tree(Position& pos, istringstream& is);

	private:
		// �ċA�I�ɍőP��𒲂ׂ�B
		VMD_Pair build_tree_nega_max(Position& pos, MemoryBook& read_book, MemoryBook& write_book);

		// "position ..."��"..."�̕��������߂���B
		int feed_position_string(Position& pos, const string& line, StateInfo* states, Thread* th);

		//  ��Ճt�@�C���̓���ǖʂ����Ղ��@��
		void extend_tree_sub(Position& pos, MemoryBook& read_book, fstream& fs, const string& sfen , bool bookhit);

		// �i���̕\��
		void output_progress();

		// �����o����node��cache���Ă����B
		std::unordered_map<std::string /*sfen*/, VMD_Pair> vmd_write_cache;

		// �����o����node��cache���Ă����B
		std::unordered_set<std::string /*sfen*/> done_sfen;

		// do_move�����w������L�^���Ă����B
		std::vector<Move> lastMoves;

		// Position::do_move(),undo_move()��wrapper
		void do_move(Position& pos, Move m, StateInfo& si) { lastMoves.push_back(m);  pos.do_move(m, si); }
		void undo_move(Position& pos,Move m) { lastMoves.pop_back(); pos.undo_move(m); }

		// ��������node��/�����o����node��
		u64 total_node = 0;
		u64 total_write_node = 0;

		// build_tree_nega_max()�ŗp������/����contempt�B
		int black_contempt, white_contempt;

		// extend_tree_sub()�ŗp������/����eval�̉���
		int black_eval_limit, white_eval_limit;

		// ��������leaf�̒l�͈̔�(enable_extend_range == true�̂Ƃ��������̋@�\���L���������)
		//   extend_range1 <= lastEval <= extend_range2
		// ��leaf node(�̌���)���������������B
		bool enable_extend_range;
		int extend_range1, extend_range2;

		// extend_tree_sub()�̈�O�̎���eval�̒l
		int lastEval;
	};

	void BookTreeBuilder::output_progress()
	{
		if ((total_node % 1000) == 0)
		{
			cout << endl << total_node;
			if (total_write_node)
				cout << "|" << total_write_node;
		}
		if ((total_node % 10) == 0)
			cout << ".";
		++total_node;
	}

	// �ċA�I�ɍőP��𒲂ׂ�B
	VMD_Pair BookTreeBuilder::build_tree_nega_max(Position& pos, MemoryBook& read_book, MemoryBook& write_book)
	{
		// -- ���łɒT���ς݂ł���Ȃ�A���̂Ƃ��̒l��Ԃ��B

		auto node_sfen = pos.sfen();
		auto it_write = vmd_write_cache.find(node_sfen);
		if (it_write != vmd_write_cache.end())
			return it_write->second;

		VMD_Pair result;

		// -- ��Ղ�hit���Ȃ��ɂ���A�l�݂Ɛ錾�����A�����Ɋւ��Ă͏����ł���̂ł��ꑊ���̒l��Ԃ��K�v������B

		// ���ǖʂŋl��ł���
		if (pos.is_mated())
		{
			result = VMD_Pair(mated_in(0), MOVE_NONE, DEPTH_ZERO);
			goto RETURN_RESULT;
		}

		{
			// ���ǖʂŐ錾�����ł���B
			// ��Ճt�@�C����MOVE_WIN�����ꂽ�Ƃ��̉��߂��K�肵�Ă��Ȃ��̂ł����ł͓���Ȃ����Ƃɂ���B
			if (pos.DeclarationWin() != MOVE_NONE)
			{
				result = VMD_Pair(mate_in(1), MOVE_NONE, DEPTH_ZERO);
				goto RETURN_RESULT;
			}

			// �����̌��o�Ȃǂ��K�v�ł������B
			// �ʂ̋ǖʂ���˂����񂾏ꍇ�͐����ɂȂ�Ȃ��\�������邪�A��Ղ͈̔͂łȂ��Ȃ��N������̂ł��Ȃ��̂ł܂�������B
			auto draw_type = pos.is_repetition(MAX_PLY);
			if (draw_type != REPETITION_NONE)
			{
				// ���̎��̈�肪�~�����C�͂���Bis_repetition()���Ԃ��ė~�����C�͂���̂����A
				// StateInfo���w�����ۑ����Ă��Ȃ��ĕԂ��Ȃ��̂��c�B(�L�ցM)
				// ����̂��߂�����"KEEP_LAST_MOVE"��define����̂�����ƌ����ȁc�B���O�Ŏ����c�B

				// �����
				switch (draw_type)
				{
					// ������-1�ɂ��Ă��܂��������A���œ�����Ղ�p����̂ł���͂ł��Ȃ�(�L�ցM)
					// �����A������Ƃ��Ȃ��ƌ�肾�ƕK�������_���ɂȂ��Ă��܂��c�B
					// ������)
					// rootColor��BLACK,WHITE�̎����ꂼ��p��VMD��Ԃ��ׂ��B
					// value = rootColor == pos.side_to_move() ? -comtempt : +comtempt;
					// �݂����Ȋ����B
					// �����A���ԂŐ�����comtempt��30(������eval��-30�����ɂ���)���Ƃ��āA
					// ��肾����ƌ����ĐϋɓI�ɐ�����_���Ă��c�݂����Ȗ��͂���B
					// ��Տ�́A����contempt = 0 , ����contempt = 70���炢�������悤�Ɏv���B

					// PawnValue/100���|���Đ��K�����鏈���͂����ł͂��Ȃ����Ƃɂ���B�ǂ�ȒT�����Ő������ꂽ��Ղ��킩��Ȃ��̂�
					// book���eval�̒l�͐��K������Ă�����̂Ɖ��肷��B
					// (makebook think�R�}���h���Ɛ��K������Ȃ����c�܂��������낤..)

				case REPETITION_DRAW:
					// �����ǖʂ����ۂɌ����āA���̒���̎w������擾����B
				{
					auto key = pos.key();
					int i = 0;
					auto* statePtr = pos.state();
					do {
						// 2�肸�k��
						statePtr = statePtr->previous->previous;
						i += 2;
					} while (key != statePtr->key());
					// i��O������ǖʂł��邱�Ƃ��킩�����̂ŁA���̎��̎w����𓾂�B

					// ��) 4��O�̋ǖʂ�key()�������Ȃ�4��O����z���Đ���肪�����B���Ȃ킿�AlastMoves�̌�납��5�ڂ̎w����Ő����ǖʂɓ˓����Ă���̂�
					// ���̎��̎w����(4��O�̎w����)���A�����̎��̈��̂͂��c�B
					auto draw_move = lastMoves[lastMoves.size() - i];
					// ���̕��ʂ̐����ȊO�̃P�[�X�ł�������Ɣ񍇖@��ɂȂ�\���������āc�B

					//  contempt * Eval::PawnValue / 100 �Ƃ��������͂��Ȃ��B
					// ���DB��makebook think�R�}���h�ō쐬����Ă��āA���̐��K���͂��łɂȂ���Ă���B

					// ���ǖʂ̎�Ԃ����ĕ��������߂Ȃ��Ƃ����Ȃ��B
					auto stm = pos.side_to_move();
					result = VMD_Pair(
						(Value)(stm == BLACK ? -black_contempt : +black_contempt) /*����comtempt */, draw_move, DEPTH_ZERO,
						(Value)(stm == WHITE ? -white_contempt : +white_contempt) /*����comtempt */, draw_move, DEPTH_ZERO
					);
					goto RETURN_RESULT;
				}

				case REPETITION_INFERIOR: result = VMD_Pair(-VALUE_SUPERIOR, MOVE_NONE, DEPTH_ZERO); goto RETURN_RESULT;
				case REPETITION_SUPERIOR: result = VMD_Pair(VALUE_SUPERIOR, MOVE_NONE, DEPTH_ZERO); goto RETURN_RESULT;
				case REPETITION_WIN: result = VMD_Pair(mate_in(MAX_PLY), MOVE_NONE, DEPTH_ZERO); goto RETURN_RESULT;
				case REPETITION_LOSE: result = VMD_Pair(mated_in(MAX_PLY), MOVE_NONE, DEPTH_ZERO); goto RETURN_RESULT;

					// �������Ă����Ȃ���clang�Ōx�����o��B
				case REPETITION_NONE:
				case REPETITION_NB:
					break;

				}
			}

			// -- ��Ղ�hit����̂��H

			auto it_read = read_book.find(pos);
			if (it_read == nullptr || it_read->size() == 0)
				// ����node�ɂ��āA����ȏ�A���������ł��Ȃ��ł�����B
			{
				// �ۑ����鉿�l���Ȃ������H
				result = VMD_Pair(VALUE_NONE, MOVE_NONE, DEPTH_ZERO);
				goto RETURN_RESULT;
			}

			// -- ����node��W�J����B

			// �V�����ق��̒�Ճt�@�C���ɓo�^���ׂ�����node�̌���
			auto list = PosMoveListPtr(new PosMoveList());

			StateInfo si;

			// ����node�̍őP��BrootColor��BLACK,WHITE�p�A���ꂼ��B
			VMD best[COLOR_NB];

			// ����best.value������w����ł���΂��̎w�����best.move,best.depth���X�V����B
			auto add_list = [&](Book::BookPos& bp, Color c /* ����node��Color */, bool update_list)
			{
				ASSERT_LV3(bp.value != VALUE_NONE);

				// ��Ղɓo�^����B
				bp.num = 1; // �o���p�x��1�ɌŒ肵�Ă����Ȃ���sort�̂Ƃ��ɕ]���l�ō~���ɕ��΂Ȃ��č���B

				if (update_list)
					list->push_back(bp);

				// ����node��bestValue���X�V������A�����return�̂Ƃ��ɕԂ��K�v������̂ŕۑ����Ă����B
				VMD vmd((Value)bp.value, bp.bestMove, (Depth)bp.depth);

				// �l���������̂ł���node��best���X�V�B
				if (best[c].value < vmd.value)
					best[c] = vmd;
			};

			// ���ׂĂ̍��@���1��i�߂�B
			// 1) �q�m�[�h���Ȃ��@���@�v�l�����X�R�A������Ȃ炻��ő�p�@�Ȃ���΁@���̎q�m�[�h�ɂ��Ă͍l���Ȃ�
			// 2) �q�m�[�h������@���@���̃X�R�A���ՂƂ��ēo�^

			for (const auto& m : MoveList<LEGAL_ALL>(pos))
			{
				// ���̎w��������ǂ�
				this->do_move(pos, m, si);
				auto vmd_pair = build_tree_nega_max(pos, read_book, write_book);
				this->undo_move(pos, m);

				for (auto color : COLOR)
				{
					// root_color�����p��best�̍X�V�ƌ��p��best�̍X�V�Ƃ��A�ʂɕK�v�ł���B(DRAW_VALUE�̏����̂���)
					auto& vmd = color == BLACK ? vmd_pair.black : vmd_pair.white;

					// color�����̋ǖʂ̎��(��root_color)�ł���Ƃ���������node�̌��胊�X�g���X�V����B
					// �����łȂ��Ƃ���best�̍X�V�͍s���B
					auto update_list = color == pos.side_to_move();

					// �qnode�̒T�����ʂ����o���B
					// depth�́A���̐��bestMove��H���Ă����Ƃ�leaf node�܂ŉ��肠�邩�Ƃ����l�Ȃ̂ł����Œ�Ղ��r�؂��Ȃ�DEPTH_ZERO�B
					auto value = vmd.value;
					auto nextMove = vmd.move;
					auto depth = vmd.depth + 1;

					if (value == VALUE_NONE)
					{
						// �q���Ȃ�����

						// ��Ղɂ��̎w���肪�������̂ł���΁A������R�s�[���Ă���B�Ȃ���΂��̎w����ɂ��Ă͉����������Ȃ��B
						auto it = std::find_if(it_read->begin(), it_read->end(), [m](const auto& x) { return x.bestMove == m; });
						if (it != it_read->end())
						{
							it->depth = DEPTH_ZERO; // depth�͂�����leaf�Ȃ̂�0����
							add_list(*it, color, update_list);
						}
					}
					else
					{
						// �q���������̂ł��̒l�Œ�Ղ�o�^�������B���̏ꍇ�A����node�̎v�l�̎w�����hit���Ă悤�Ɗ֌W�Ȃ��B

						// nega max�Ȃ̂ŕ����𔽓]������
						value = -value;

						// �l�݂̃X�R�A��root����l�݂܂ł̋����ɉ����ăX�R�A���C�����Ȃ��Ƃ����Ȃ��B
						if (value >= VALUE_MATE)
							--value;
						else if (value <= -VALUE_MATE)
							++value;

						//ASSERT_LV3(nextMove != MOVE_NONE);

						Book::BookPos bp(m, nextMove, value, depth, 1);
						add_list(bp, color, update_list);
					}
				}
			}

			// ����node�ɂ��Ē��׏I������̂Ŋi�[
			std::stable_sort(list->begin(), list->end());
			write_book.book_body[pos.sfen()] = list;

			// 10 / 1000 node ���������̂Ői�����o��
			output_progress();

#if 0
			// �f�o�b�O�̂��߂ɂ���node�Ɋւ��āA�����o���\��̒�Տ���\�������Ă݂�B

			cout << pos.sfen() << endl;
			for (const auto& it : *list)
			{
				cout << it << endl;
			}
#endif

			result = VMD_Pair(best);
		}

	RETURN_RESULT:

		// ����node�̏���write_cache�ɕۑ�
		vmd_write_cache[node_sfen] = result;

		return result;
	}

	// ���game tree�𐶐�����@�\
	void BookTreeBuilder::build_tree(Position & pos, istringstream & is)
	{
		// ��Ճt�@�C����
		// Option["book_file"]�ł͂Ȃ��A�����Ŏw�肵�����̂������Ώۂł���B
		string read_book_name, write_book_name;
		is >> read_book_name >> write_book_name;

		// �����Ώۃt�@�C�����̏o��
		cout << "makebook build_tree .." << endl;
		cout << "read_book_name   = " << read_book_name << endl;
		cout << "write_book_name  = " << write_book_name << endl;

		MemoryBook read_book, write_book;
		if (read_book.read_book(read_book_name) != 0)
		{
			cout << "Error! : failed to read " << read_book_name << endl;
			return;
		}

		// ��Ղł͐�ΐ����������}���̐ݒ�
		int black_contempt =  50;  // ��葤�̐����� -50�Ƃ݂Ȃ�
		int white_contempt = 150;  // ��葤�̐�����-150�Ƃ݂Ȃ�

		string token;
		while ((is >> token))
		{
			if (token == "black_contempt")
				is >> black_contempt;
			else if (token == "white_contempt")
				is >> white_contempt;
		}

		cout << "black_contempt = " << black_contempt << endl;
		cout << "white_contempt = " << white_contempt << endl;

		// �����ǖʂ���(depth 10000�ł͂Ȃ����̂�)�H����game tree���\�z����B

		StateInfo si;
		pos.set_hirate(&si, Threads.main());
		this->lastMoves.clear();

		total_node = 0;
		total_write_node = 0;
		vmd_write_cache.clear();

		this->black_contempt = black_contempt;
		this->white_contempt = white_contempt;

		build_tree_nega_max(pos, read_book, write_book);
		cout << endl;

		// �����o��
		cout << "write " << write_book_name << endl;
		write_book.write_book(write_book_name, true);

		cout << "done." << endl;
	}

	// ----------------------------
	//  ��Ճt�@�C���̓���ǖʂ����Ղ��@��
	// ----------------------------

	void BookTreeBuilder::extend_tree_sub(Position & pos, MemoryBook & read_book, fstream & fs, const string & sfen , bool book_hit)
	{
		// �����ɓ��B�����ǖʂ͎v�l�ΏۂƂ��Ă͂Ȃ�Ȃ��B
		auto draw_type = pos.is_repetition(MAX_PLY);
		if (draw_type == REPETITION_DRAW)
			return;
		// ���@����ȊO�̔����͑傫�ȃX�R�A�����͂��ŁA���O�����͂��B

		// �l�݁A�錾�����̋ǖʂ��v�l�ΏۂƂ��Ă͂Ȃ�Ȃ��B
		if (pos.is_mated() || pos.DeclarationWin() != MOVE_NONE)
			return;
		// ���@���O��node�ő傫�ȃX�R�A�����͂������珜�O�����͂������B

		// ����node�������ς݂ł��邩
		// ���@���̏����x�����Ȃ�..pos.key()�ŏ\���Ȃ悤�ȋC�����邪�c�B
		string this_sfen = pos.sfen();
		if (done_sfen.find(this_sfen) != done_sfen.end())
			return;
		done_sfen.insert(this_sfen);

		// ��ՂɃq�b�g���Ȃ������̂ł���΁A������leaf node�̎��̋ǖʂȂ̂ŁA
		// ���̋ǖʂ܂ł�"startpos moves..."�������o���B
		auto it_read = read_book.find(pos);
		if (it_read == nullptr || it_read->size() == 0)
		{
			// ���O��node�Œ�Ղ̎w������w�����̂ł͂Ȃ��Ȃ�A��B
			if (!book_hit)
				return;

			// ���O��node�Œ�Ղ̎w������w���āA���A�����Œ�Ղ�hit���Ȃ������̂�
			// ���������tree��leaf node�̈���node�ł��邱�Ƃ��m�肵���B

			// ���O��node��score�� +1,-1 �ł���Ȃ�A����͐����X�R�A���ƍl������̂ŁA���̂Ƃ��ɂ̂�
			// ���̋ǖʂ���������B(Learner::search()�̎d�l���_�T���āA"makebook think"�R�}���h�͐����̂Ƃ�+1�����肤��c)
			// ���@Contempt�̒l�͖������ď��0�ɂȂ�悤�ɏC�������B[2019/05/19]

			// ���̏������͒P�����ł��邪�A������ǉ����邩���m��Ȃ��̂ŉߏ�ȒP�����͂��Ȃ��B
			bool extend = 
				(this->enable_extend_range && extend_range1 <= this->lastEval && this->lastEval <= extend_range2) ||
				(!this->enable_extend_range);

			// ���̋ǖʂɓ��B����܂ł�"startpos moves ..."���Ƃ�܏o�́B
			if (extend)
			{
				fs << sfen << endl;
				//fs << pos.sfen() << endl;

				total_write_node++;
			}

			return;
		}

		// -- ��Ղɓo�^����Ă��邱��node�̎w�����W�J����B

		StateInfo si;
		auto turn = pos.side_to_move();

		for (const auto& m : MoveList<LEGAL_ALL>(pos))
		{
			// ��Ղ̎w����ȊO�̎w����ł��A���̋ǖʂŒ�Ղ�hit����w�����T���K�v������B

			auto it = std::find_if(it_read->begin(), it_read->end(), [m](const auto & x) { return x.bestMove == m; });
			// ��Ղ�hit�����̂�
			bool book_hit = it != it_read->end();

			if (book_hit)
			{
				// eval_limit�𒴂���}������W�J����B
				if ((turn == BLACK && it->value >= black_eval_limit) ||
					(turn == WHITE && it->value >= white_eval_limit))
					this->lastEval = it->value;
				else
					continue; // ���̎}�͂��̎��_�Ő��H��Ȃ�
			}
			else {
				// ��Ղ̎w����ł͂Ȃ����A����node�Œ�Ղ�hit����Ȃ�H���ė~�����B
				this->lastEval = 0;
			}
			this->do_move(pos,m, si);
			extend_tree_sub(pos, read_book, fs, sfen + " " + to_usi_string(m) , book_hit);
			this->undo_move(pos,m);
		}

		output_progress();
	}

	// "position ..."��"..."�̕��������߂���B
	int BookTreeBuilder::feed_position_string(Position & pos, const string & line, StateInfo * states, Thread * th)
	{
		pos.set_hirate(&states[0], th);
		// ��������Aline�Ŏw�肳�ꂽ"startpos moves"..��ǂݍ���ł��̋ǖʂ܂Ői�߂�B
		// �����ł�"sfen"�ŋǖʂ͎w��ł��Ȃ����̂Ƃ���B
		this->lastMoves.clear();

		stringstream ss(line);
		string token;
		while ((ss >> token))
		{
			if (token == "startpos" || token == "moves")
				continue;

			Move move = USI::to_move(pos, token);
			if (token == "sfen" || move == MOVE_NONE || !pos.pseudo_legal(move) || !pos.legal(move))
			{
				cout << "Error ! : " << line << " unknown token = " << token << endl;
				return 1;
			}
			this->do_move(pos,move, states[pos.game_ply()]);
		}
		return 0; // �ǂݍ��ݏI��
	}

	// ��Ճt�@�C����ǂݍ���ŁA�w��ǖʂ���[�@�肷�邽�߂ɕK�v�Ȋ����𐶐�����B
	void BookTreeBuilder::extend_tree(Position & pos, istringstream & is)
	{
		string read_book_name, read_sfen_name, write_sfen_name;
		is >> read_book_name >> read_sfen_name >> write_sfen_name;

		// ��������]���l�̉���
		int black_eval_limit = -50, white_eval_limit = -150;

		// ��������leaf�̕]���l�͈̔�(�����X�R�A�̋ǖʂ݂̂���������ꍇ�ɗp����)
		bool enable_extend_range = false;
		int extend_range1, extend_range2;

		string token;
		while ((is >> token))
		{
			if (token == "black_eval_limit")
				is >> black_eval_limit;
			else if (token == "white_eval_limit")
				is >> white_eval_limit;
			else if (token == "extend_range")
			{
				enable_extend_range = true;
				is >> extend_range1 >> extend_range2;
			}
		}

		// �����Ώۃt�@�C�����̏o��
		cout << "makebook extend tree .." << endl;
		
		cout << "read_book_name   = " << read_book_name << endl;
		cout << "read_sfen_name  = " << read_sfen_name << endl;
		cout << "write_sfen_name  = " << write_sfen_name << endl;

		cout << "black_eval_limit = " << black_eval_limit << endl;
		cout << "white_eval_limit = " << white_eval_limit << endl;

		if (enable_extend_range)
			cout << "extend_range = [" << extend_range1 << "," << extend_range2 << "]" << endl;

		// read_book_name  : ��˂��牤�̒�Ռ`��(�g���q.db)
		// read_sfen_name  : USI��position�`���B��:"startpos moves ..."
		// write_sfen_name : ����B

		MemoryBook read_book;
		if (read_book.read_book(read_book_name) != 0)
		{
			cout << "Error! : failed to read " << read_book_name << endl;
			return;
		}
		vector<string> lines;
		read_all_lines(read_sfen_name, lines);

		// �����ǖʂ���(depth 10000�ł͂Ȃ����̂�)�H����game tree���\�z����B

		fstream fs;
		fs.open(write_sfen_name , ios::out);

		total_node = 0;
		total_write_node = 0;
		this->done_sfen.clear();

		this->black_eval_limit = black_eval_limit;
		this->white_eval_limit = white_eval_limit;
		this->enable_extend_range = enable_extend_range;
		this->extend_range1 = extend_range1;
		this->extend_range2 = extend_range2;

		// �����蒷�������A�H�킹�Ȃ����c�B
		std::vector<StateInfo, AlignedAllocator<StateInfo>> states(1024);

		for (int i = 0; i < (int)lines.size(); ++i)
		{
			auto& line = lines[i];
			if (line == "startpos")
				line = "startpos moves";

			cout << "extend[" << i << "] : " << line << endl;
			feed_position_string(pos, line, &states[0], Threads.main());
			//pos.set(line, &si, Threads.main());

			this->lastEval = 0;
			extend_tree_sub(pos, read_book, fs, line , true);
		}
		fs.close();
		cout << endl;
		cout << "done." << endl;
	}

	// ��Ղ̖��������@��
	/*
		�ۑ�ǖʂ��玩���I�ɒ�Ղ��@���Ă����B

		// ��x�ڂ����ۑ�ǖʂ܂ł��v�l������B(�ۑ�ǖʂ܂ł̒�Ղ��@��Ȃ��Ă����Ȃ�A���̓���͕s�v)
		MultiPV 4
		makebook think book/kadai_sfen.txt book/book_test.db depth 8 startmoves 1 moves 32
		// ���@�����߂悤�B�s�v����B

		// ���̂��ƁA�ȉ��𖳌��ɌJ��Ԃ��B
		MultiPV 4
		makebook extend_tree book/book_test.db book/kadai_sfen.txt book/think_sfen.txt
		makebook think book/think_sfen.txt book/book_test.db depth 8 startmoves 1 moves 32

		�����̂Q�𓝍������R�}���h���쐬����

		��)
		makebook endless_extend_tree book/book_test.db book/kadai_sfen book/think_sfen.txt depth 8 startmoves 1 moves 32 loop 10 nodes 100000
	*/
	void BookTreeBuilder::endless_extend_tree(Position& pos, istringstream& is)
	{
		string read_book_name, read_sfen_name, think_sfen_name;
		is >> read_book_name >> read_sfen_name >> think_sfen_name;

		cout << "endless_extend_tree" << endl;

		cout << "read_book_name   = " << read_book_name << endl;
		cout << "read_sfen_name  = " << read_sfen_name << endl;
		cout << "write_sfen_name  = " << think_sfen_name << endl;

		string token;
		int depth = 8, start_moves = 1, end_moves = 32 , iteration = 256;
		int black_eval_limit = -50, white_eval_limit = -150;
		uint64_t nodes = 0; // �w�肪�Ȃ����0�ɂ��Ƃ��Ȃ���..
		bool enable_extend_range = false;
		int extend_range1, extend_range2;

		while ((is >> token))
		{
			if (token == "depth")
				is >> depth;
			else if (token == "startmoves")
				is >> start_moves;
			else if (token == "moves")
				is >> end_moves;
			else if (token == "loop")
				is >> iteration;
			else if (token == "black_eval_limit")
				is >> black_eval_limit;
			else if (token == "white_eval_limit")
				is >> white_eval_limit;
			else if (token == "nodes")
				is >> nodes;
			else if (token == "extend_range")
			{
				enable_extend_range = true;
				is >> extend_range1 >> extend_range2;
			}
		}

		cout << "startmoves " << start_moves << " moves " << end_moves << " nodes " << nodes << endl;
		cout << "loop = " << iteration << endl;
		cout << "black_eval_limit = " << black_eval_limit << endl;
		cout << "white_eval_limit = " << white_eval_limit << endl;
		if (enable_extend_range)
			cout << "extend_range = [" << extend_range1 << "," << extend_range2 << "]" << endl;

		// �R�}���h�̎��s
		auto do_command = [&](string command)
		{
			cout << "> makebook " + command << endl;

			// "makebook"�R�}���h�������ɃR�}���h�e�L�X�g�o�R�ňڏ����Ă��܂��B
			istringstream iss(command);
			makebook_cmd(pos, iss);
		};

		for (int i = 0; i < iteration; ++i)
		{
			cout << "makebook engless_extend_tree : iteration " << i << endl;

			string command;
#if 0
			if (i == 0)
			{
				// ���񂾂�
				// "makebook think book/kadai_sfen.txt book/book_test.db depth 8 startmoves 1 moves 32"

				command = "think " + read_sfen_name + " " + read_book_name + " depth " + to_string(depth)
					+ " startmoves " + to_string(start_moves) + " moves " + to_string(end_moves) + " nodes " + to_string(nodes);
				do_command(command);
			}
			else
#endif
			{
				// 2��ڈȍ~

				// "makebook extend_tree book/book_test.db book/kadai_sfen.txt book/think_sfen.txt"
				// "makebook think book/think_sfen.txt book/book_test.db depth 8 startmoves 1 moves 32"

				command = "extend_tree " + read_book_name + " " + read_sfen_name + " " + think_sfen_name +
					" black_eval_limit " + to_string(black_eval_limit) + " white_eval_limit " + to_string(white_eval_limit) +
					(enable_extend_range ? " extend_range " + to_string(extend_range1) + " " + to_string(extend_range2):"");
				do_command(command);


				// �����Ŏv�l�Ώۋǖʂ��Ȃ��Ȃ��Ă���ΏI������K�v������B
				// moves���w�肵�Ă���̂�think_sfen_name�̃t�@�C������ɂȂ�Ƃ͌���Ȃ��āA���̏I���������c�B
				// �Ȃ̂ŁA���Ȃ�(�L�ցM)

				command = "think " + think_sfen_name + " " + read_book_name + " depth " + to_string(depth)
					+ " startmoves " + to_string(start_moves) + " moves " + to_string(end_moves) + +" nodes " + to_string(nodes);
				do_command(command);

			}
		}

	}

}

namespace Book
{
	// 2019�N�ȍ~�ɍ����makebook�g���R�}���h
	// "makebook XXX"�R�}���h�BXXX�̕�����"build_tree"��"extend_tree"������B
	// ���̊g���R�}���h������������A���̊֐��͔�0��Ԃ��B
	int makebook2019(Position& pos, istringstream& is, const string& token)
	{
		if (token == "build_tree")
		{
			BookTreeBuilder builder;
			builder.build_tree(pos, is);
			return 1;
		}

		if (token == "extend_tree")
		{
			BookTreeBuilder builder;
			builder.extend_tree(pos, is);
			return 1;
		}

		if (token == "endless_extend_tree")
		{
			BookTreeBuilder builder;
			builder.endless_extend_tree(pos, is);
			return 1;
		}

		return 0;
	}
}

#endif
