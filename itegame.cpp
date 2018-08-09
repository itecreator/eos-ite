/**
*                         ______ _______ ______ 
*                        |_   _|__   __|  ____|
*                          | |    | |  | |__   
*                          | |    | |  |  __|  
*                         _| |_   | |  | |____ 
*                        |_____|  |_|  |______|
*                    
*
*        _______ _____ ____   _____ _____     _______ ___  _    _ _      
*      |__   __|__  _/ __ \ / ____|_   _|   |__   __|__ \ | |  | | |     
*          | |    | || |  | | |      | |        | |     ) | |  | | |     
*          | |    | || |  | | |      | |        | |    / /| |  | | |     
*          | |   _| || |__| | |____ _| |_       | |   / /_| |__| | |____ 
*          |_|  |_____\____/ \_____|_____|      |_|  |____|\____/|______|
*                                                                  
*
*          
*         Mirror, mirror on the wall, who's the hottest of them all ?
*
*
*    _                         _   _                      _                  _   
*  (_)                       | | | |                    | |                | |  
*    _ _ __    _ __ ___   __ _| |_| |__   __      _____  | |_ _ __ _   _ ___| |_ 
*  | | '_ \  | '_ ` _ \ / _` | __| '_ \  \ \ /\ / / _ \ | __| '__| | | / __| __|
*  | | | | | | | | | | | (_| | |_| | | |  \ V  V /  __/ | |_| |  | |_| \__ \ |_ 
*  |_|_| |_| |_| |_| |_|\__,_|\__|_| |_|   \_/\_/ \___|  \__|_|   \__,_|___/\__|
*
*
*/
#include <eosiolib/currency.hpp>
#include <math.h>

#define SOPHON S(4, SOPHON)
#define SATOSHI S(4, SATOSHI)
#define GAME_SYMBOL S(4, EOS)
#define FEE_ACCOUNT N(itewhitehole)
#define DEV_FEE_ACCOUNT N(jekyllisland)
#define TOKEN_CONTRACT N(eosio.token)

typedef double real_type;

using namespace eosio;
using namespace std;

class itegame : public contract
{
public:
  // parameters
  const uint64_t init_base_balance = 64 * 1024 * 1024ll / 100; // 初始化筹码
  const uint64_t init_quote_balance = 1 * 10000 * 10000ll;     // 初始保证金
  const uint64_t burn_price_ratio = 90;                        // 销毁价格比例
  const uint64_t end_prize_times = 10;                         // 终极大奖收益倍数
  const uint64_t end_prize_ratio = 10;                         // 终极大奖瓜分奖池的比例
  const uint64_t good_ending_ratio = 50;                       // 销毁智子总数 占 总智子数的比例。达到这个比例 游戏结束
  const uint64_t bad_ending_ratio = 90;                        // 已激活智子总数 占 当前有效智子数 (不含已销毁) 的比例。达到这个比例 游戏结束
  const uint64_t air_drop_step = 1000;                         // 空投奖励的间隔
  const uint64_t action_limit_time = 15;                       // 操作冷却时间(s) 
  const uint64_t max_operate_amount_ratio = 1;                 // 每次可操作的智子数量, 不大于当前有效智子数 (不含已销毁) 的比例。(防止大户无脑绝杀，游戏变成猝死局)
  const uint64_t game_start_time = 1533970800000 * 1000;       // 游戏开始时间 2018-08-11 15:00:00
  const uint64_t max_end_prize = 5000 * 10000;                 // 终极大奖 设定为最大 5000 EOS

  itegame(account_name self)
      : contract(self),
        _global(_self, _self),
        _games(_self, _self),
        _market(_self, _self)
  {
    // Create a new global if not exists
    auto gl_itr = _global.begin();
    if (gl_itr == _global.end())
    {
      gl_itr = _global.emplace(_self, [&](auto &gl) {
        gl.gameid = 0;
        gl.air_drop_step = air_drop_step;
        gl.burn_price_ratio = burn_price_ratio;
        gl.end_prize_times = end_prize_times;
        gl.end_prize_ratio = end_prize_ratio;
        gl.good_ending_ratio = good_ending_ratio;
        gl.bad_ending_ratio = bad_ending_ratio;
        gl.action_limit_time = action_limit_time;
        gl.max_operate_amount_ratio = max_operate_amount_ratio;
        gl.max_end_prize = max_end_prize;
        gl.game_start_time = game_start_time;
      });
    }

    // Create a new game if not exists
    auto game_itr = _games.find(gl_itr->gameid);
    if (game_itr == _games.end())
    {
      game_itr = _games.emplace(_self, [&](auto &new_game) {
        new_game.gameid = gl_itr->gameid;
        new_game.init_max = init_base_balance;
        new_game.total_alive = init_base_balance;
        new_game.quote_balance.amount = init_quote_balance;
        new_game.init_quote_balance.amount = init_quote_balance;
      });
    }

    // Create a new market if not exists
    auto market_itr = _market.find(gl_itr->gameid);
    if (market_itr == _market.end())
    {
      market_itr = _market.emplace(_self, [&](auto &m) {
        m.gameid = gl_itr->gameid;
        m.supply.amount = 100000000000000ll;
        m.supply.symbol = SATOSHI;
        m.base.balance.amount = init_base_balance;
        m.base.balance.symbol = SOPHON;
        m.quote.balance.amount = init_quote_balance;
        m.quote.balance.symbol = GAME_SYMBOL;
      });
    }
  }

  void transfer(account_name from, account_name to, asset quantity, string memo)
  {
    if (from == _self || to != _self)
    {
      return;
    }

    eosio_assert(current_time() > game_start_time, "The game will start at 2018-08-11 15:00:00");

    eosio_assert(quantity.is_valid(), "Invalid token transfer");
    eosio_assert(quantity.amount > 0, "Quantity must be positive");

    // only accepts GAME_SYMBOL for buy
    if (quantity.symbol == GAME_SYMBOL)
    {
      buy(from, quantity);
    }
  }

  void buy(account_name account, asset quant)
  {
    require_auth(account);
    eosio_assert(quant.amount > 0, "must purchase a positive amount");

    auto gl_itr = _global.begin();

    auto market_itr = _market.find(gl_itr->gameid);

    auto fee = quant;
    fee.amount = (fee.amount + 199) / 200; /// .5% fee (round up)
    auto action_total_fee = fee;

    auto quant_after_fee = quant;
    quant_after_fee.amount -= fee.amount;

    if (fee.amount > 0)
    {
      auto dev_fee = fee;
      dev_fee.amount = fee.amount * 30 / 100;
      fee.amount -= dev_fee.amount;

      action(
          permission_level{_self, N(active)},
          TOKEN_CONTRACT, N(transfer),
          make_tuple(_self, FEE_ACCOUNT, fee, string("buy fee")))
          .send();

      if (dev_fee.amount > 0)
      {
        action(
            permission_level{_self, N(active)},
            TOKEN_CONTRACT, N(transfer),
            make_tuple(_self, DEV_FEE_ACCOUNT, dev_fee, string("dev fee")))
            .send();
      }
    }

    int64_t bytes_out;

    _market.modify(market_itr, 0, [&](auto &es) {
      bytes_out = es.convert(quant_after_fee, SOPHON).amount;
    });

    eosio_assert(bytes_out > 0, "must reserve a positive amount");

    auto game_itr = _games.find(gl_itr->gameid);

    // max operate amount limit
    auto max_operate_amount = game_itr->total_alive / 100 * max_operate_amount_ratio;
    eosio_assert(bytes_out < max_operate_amount, "must reserve less than max operate amount");

    // burn 1 %
    int64_t burn = bytes_out / 100;

    _games.modify(game_itr, 0, [&](auto &game) {
      // counter limit
      if (quant_after_fee.amount > 50000)
      {
        game.counter++;
      }
      game.total_burn += burn;
      game.total_alive -= burn;
      game.total_reserved += bytes_out;
      game.quote_balance += quant_after_fee;
    });

    user_resources_table userres(_self, account);

    auto res_itr = userres.find(gl_itr->gameid);

    if (res_itr == userres.end())
    {
      res_itr = userres.emplace(account, [&](auto &res) {
        res.gameid = gl_itr->gameid;
        res.owner = account;
        res.hodl = bytes_out;
        res.action_count++;
        res.fee_amount += action_total_fee;
        res.out += quant;
      });

      // add new player into players table
      player_index _players(_self, gl_itr->gameid);

      _players.emplace(_self, [&](auto &new_player) {
        new_player.player_account = account;
      });
    }
    else
    {
      // time limit
      auto time_diff = (current_time() - res_itr->last_action_time) / 1000 / 1000;
      eosio_assert(time_diff > action_limit_time, "please wait a moment");

      userres.modify(res_itr, account, [&](auto &res) {
        res.hodl += bytes_out;
        res.last_action_time = current_time();
        res.action_count++;
        res.fee_amount += action_total_fee;
        res.out += quant;
      });
    }
    trigger_air_drop(account, quant_after_fee);
    trigger_game_over(account, quant_after_fee);
  }

  void sell(account_name account, int64_t bytes)
  {
    require_auth(account);
    eosio_assert(bytes > 0, "cannot sell negative byte");

    auto gl_itr = _global.begin();

    user_resources_table userres(_self, account);
    auto res_itr = userres.find(gl_itr->gameid);

    eosio_assert(res_itr != userres.end(), "no resource row");
    eosio_assert(res_itr->hodl >= bytes, "insufficient quota");

    // time limit
    auto time_diff = (current_time() - res_itr->last_action_time) / 1000 / 1000;
    eosio_assert(time_diff > action_limit_time, "please wait a moment");

    auto game_itr = _games.find(gl_itr->gameid);

    // max operate amount limit
    // auto max_operate_amount = game_itr->total_alive / 100 * max_operate_amount_ratio;
    // eosio_assert(bytes < max_operate_amount, "must sell less than max operate amount");

    asset tokens_out;

    auto itr = _market.find(gl_itr->gameid);

    _market.modify(itr, 0, [&](auto &es) {
      tokens_out = es.convert(asset(bytes, SOPHON), GAME_SYMBOL);
    });

    eosio_assert(tokens_out.amount > 0, "must payout a positive amount");

    // burn 1 %
    auto burn = bytes / 100;

    auto max = game_itr->quote_balance - game_itr->init_quote_balance;

    if (tokens_out > max)
    {
      tokens_out = max;
    }

    _games.modify(game_itr, 0, [&](auto &game) {
      // counter limit
      if (tokens_out.amount > 50000)
      {
        game.counter++;
      }
      game.total_burn += burn;
      game.total_alive -= burn;
      game.total_reserved -= bytes;
      game.quote_balance -= tokens_out;
    });

    auto fee = (tokens_out.amount + 199) / 200; /// .5% fee (round up)
    auto action_total_fee = fee;

    auto quant_after_fee = tokens_out;
    quant_after_fee.amount -= fee;

    userres.modify(res_itr, account, [&](auto &res) {
      res.hodl -= bytes;
      res.last_action_time = current_time();
      res.action_count++;
      res.fee_amount += asset(action_total_fee, GAME_SYMBOL);
      res.in += tokens_out;
    });

    action(
        permission_level{_self, N(active)},
        TOKEN_CONTRACT, N(transfer),
        make_tuple(_self, account, quant_after_fee, string("sell payout")))
        .send();

    if (fee > 0)
    {
      auto dev_fee = fee;
      dev_fee = fee * 30 / 100;
      fee -= dev_fee;

      action(
          permission_level{_self, N(active)},
          TOKEN_CONTRACT, N(transfer),
          make_tuple(_self, FEE_ACCOUNT, asset(fee, GAME_SYMBOL), string("sell fee")))
          .send();

      if (dev_fee > 0)
      {
        action(
            permission_level{_self, N(active)},
            TOKEN_CONTRACT, N(transfer),
            make_tuple(_self, DEV_FEE_ACCOUNT, asset(dev_fee, GAME_SYMBOL), string("dev fee")))
            .send();
      }
    }
    trigger_air_drop(account, tokens_out);
    trigger_game_over(account, tokens_out);
  }

  void destroy(account_name account, int64_t bytes)
  {
    require_auth(account);
    eosio_assert(bytes > 0, "cannot destroy negative byte");

    auto gl_itr = _global.begin();

    user_resources_table userres(_self, account);
    auto res_itr = userres.find(gl_itr->gameid);

    eosio_assert(res_itr != userres.end(), "no resource row");
    eosio_assert(res_itr->hodl >= bytes, "insufficient quota");

    // time limit
    auto time_diff = (current_time() - res_itr->last_action_time) / 1000 / 1000;
    eosio_assert(time_diff > action_limit_time, "please wait a moment");

    auto game_itr = _games.find(gl_itr->gameid);

    // max operate amount limit
    auto max_operate_amount = game_itr->total_alive / 100 * max_operate_amount_ratio;
    eosio_assert(bytes < max_operate_amount, "must destroy less than max operate amount");

    asset tokens_out;

    auto itr = _market.find(gl_itr->gameid);

    _market.modify(itr, 0, [&](auto &es) {
      tokens_out = es.convert(asset(bytes, SOPHON), GAME_SYMBOL);
    });

    asset payout = tokens_out;
    payout.amount = tokens_out.amount / 100 * burn_price_ratio;

    eosio_assert(payout.amount > 0, "must payout a positive amount");

    auto max = game_itr->quote_balance - game_itr->init_quote_balance;

    if (payout > max)
    {
      payout = max;
    }

    userres.modify(res_itr, account, [&](auto &res) {
      res.hodl -= bytes;
      res.last_action_time = current_time();
      res.action_count++;
      res.in += payout;
    });

    asset destroy_balance = tokens_out - payout;

    // change game status
    _games.modify(game_itr, 0, [&](auto &game) {
      // counter limit
      if (destroy_balance.amount > 50000)
      {
        game.counter++;
      }
      game.total_burn += bytes;
      game.total_alive -= bytes;
      game.total_reserved -= bytes;
      game.quote_balance -= payout;
      game.destroy_balance += destroy_balance;
    });

    // transfer payout to destroyer
    action(
        permission_level{_self, N(active)},
        TOKEN_CONTRACT, N(transfer),
        make_tuple(_self, account, payout, string("destroy payout")))
        .send();

    trigger_air_drop(account, payout);
    trigger_game_over(account, payout);
  }

  void trigger_air_drop(account_name account, asset quant)
  {
    auto gl_itr = _global.begin();
    const auto game_itr = _games.find(gl_itr->gameid);

    user_resources_table userres(_self, account);
    auto res_itr = userres.find(gl_itr->gameid);

    // check airdrop
    if (game_itr->counter > 0 && game_itr->counter % air_drop_step == 0)
    {
      // air drop rule: return total fee_amount of current user, the real reward <= fee_amount
      auto reward = res_itr->fee_amount;

      auto max = game_itr->quote_balance - game_itr->init_quote_balance;

      if (reward > max)
      {
        reward = max;
      }

      userres.modify(res_itr, account, [&](auto &res) {
        res.in += reward;
        res.fee_amount = asset(0, GAME_SYMBOL);
      });

      // change market status
      auto market_itr = _market.find(gl_itr->gameid);

      // calculate the energy leakage
      int64_t bytes_out;

      _market.modify(market_itr, 0, [&](auto &es) {
        bytes_out = es.convert(reward, SOPHON).amount;
        if (bytes_out > 0)
        {
          reward = es.convert(asset(bytes_out, SOPHON), GAME_SYMBOL);
          reward = es.convert(asset(bytes_out, SOPHON), GAME_SYMBOL);
        }
      });

      // change game status
      _games.modify(game_itr, 0, [&](auto &game) {
        game.total_burn += bytes_out;
        game.total_alive -= bytes_out;
        game.total_reserved -= bytes_out;
        game.quote_balance -= reward;
        game.total_lose += bytes_out;
      });

      bonus_index _bonus(_self, gl_itr->gameid);

      // create bonus record
      _bonus.emplace(_self, [&](auto &new_bonus) {
        new_bonus.count = game_itr->counter;
        new_bonus.gameid = gl_itr->gameid;
        new_bonus.owner = account;
        new_bonus.reward = reward;
        new_bonus.lose_amount = bytes_out;
      });

      if (reward.amount > 0)
      {
        action(
            permission_level{_self, N(active)},
            TOKEN_CONTRACT, N(transfer),
            make_tuple(_self, account, reward, string("air drop reward")))
            .send();
      }
    }
  }

  void trigger_game_over(account_name account, asset quant)
  {
    auto gl_itr = _global.begin();
    auto game_itr = _games.find(gl_itr->gameid);

    bool gameover = false;

    // 1. good ending of the game: total_burn >= {good_ending_ratio} % * init_max
    auto max_burn = game_itr->init_max / 100 * good_ending_ratio;

    if (game_itr->total_burn >= max_burn)
    {
      gameover = true;
    }

    // 2. bad ending of the game:  total_reserved >= total_alive * {bad_ending_ratio}%
    auto max_reserved = game_itr->total_alive / 100 * bad_ending_ratio;

    if (game_itr->total_reserved >= max_reserved)
    {
      gameover = true;
    }

    if (gameover)
    {
      // reward = {end_prize_times} * quant , but, <= {end_prize_ratio}% * quote_balance <= max_end_prize
      auto reward = quant;
      reward.amount = reward.amount * end_prize_times;

      auto max = game_itr->quote_balance - game_itr->init_quote_balance;
      max.amount = max.amount / 100 * end_prize_ratio;

      if (reward > max)
      {
        reward = max;
      }

      if (reward.amount > max_end_prize)
      {
        reward.amount = max_end_prize;
      }

      // calculate claim price : (quote_balance - init_quote_balance - reward)/ (total_reserved + total_lose)
      //（make sure the balance of account can buy back all sell）
      auto final_balance = game_itr->quote_balance - game_itr->init_quote_balance - reward;
      eosio_assert(final_balance.amount > 0, "shit happens");

      auto claim_price = final_balance;

      auto total_hold = game_itr->total_reserved + game_itr->total_lose;
      if (total_hold > 0)
      {
        claim_price.amount = claim_price.amount / total_hold;
      }

      eosio_assert(claim_price.amount > 0, "shit happens again");

      // transfer to hero
      action(
          permission_level{_self, N(active)},
          TOKEN_CONTRACT, N(transfer),
          make_tuple(_self, account, reward, string("hero reward")))
          .send();

      // change game status
      _games.modify(game_itr, 0, [&](auto &game) {
        game.status = 1;
        game.claim_price = claim_price;
        game.quote_balance -= reward;
        game.hero = account;
        game.hero_reward = reward;
        game.end_time = current_time();
      });

      // increment global game counter
      _global.modify(gl_itr, 0, [&](auto &gl) {
        gl.gameid++;
      });

      // create new game
      _games.emplace(_self, [&](auto &new_game) {
        new_game.gameid = gl_itr->gameid;
        new_game.counter = 0;
        new_game.init_max = init_base_balance;
        new_game.total_alive = init_base_balance;
        new_game.quote_balance.amount = init_quote_balance;
        new_game.init_quote_balance.amount = init_quote_balance;
      });
    }
  }

  void claim(account_name account, int64_t gameid)
  {
    require_auth(account);

    auto gl_itr = _global.begin();
    auto game_itr = _games.find(gameid);

    eosio_assert(game_itr != _games.end(), "game 404 no found");
    eosio_assert(game_itr->status == 1, "no, pls claim after game over");

    user_resources_table userres(_self, account);
    auto res_itr = userres.find(gameid);

    eosio_assert(res_itr != userres.end(), "sorry, you are so bad");
    eosio_assert(res_itr->claim_status == 0, "get out");

    auto reward = game_itr->claim_price;
    reward.amount = reward.amount * res_itr->hodl;

    userres.modify(res_itr, account, [&](auto &res) {
      res.claim_status = 1;
    });

    if (reward.amount > 0)
    {
      action(
          permission_level{_self, N(active)},
          TOKEN_CONTRACT, N(transfer),
          make_tuple(_self, account, reward, string("claim reward")))
          .send();
    }
  }

private:
  // @abi table global i64
  struct global
  {
    uint64_t id = 0;
    uint64_t gameid;
    uint64_t air_drop_step;
    uint64_t burn_price_ratio;
    uint64_t end_prize_ratio;
    uint64_t end_prize_times;
    uint64_t good_ending_ratio;
    uint64_t bad_ending_ratio;
    uint64_t action_limit_time;
    uint64_t max_operate_amount_ratio;
    uint64_t max_end_prize;
    uint64_t game_start_time;

    uint64_t primary_key() const { return id; }

    EOSLIB_SERIALIZE(global, (id)(gameid)(air_drop_step)(burn_price_ratio)(end_prize_ratio)(end_prize_times)(good_ending_ratio)(bad_ending_ratio)(action_limit_time)(max_operate_amount_ratio)(max_end_prize)(game_start_time))
  };

  typedef eosio::multi_index<N(global), global> global_index;
  global_index _global;

  // @abi table game i64
  struct game
  {
    uint64_t gameid;
    uint64_t status;
    uint64_t counter;
    uint64_t init_max;
    uint64_t total_burn;
    uint64_t total_alive;
    uint64_t total_reserved;
    uint64_t total_lose;
    uint64_t start_time = current_time();
    uint64_t end_time;
    asset quote_balance = asset(0, GAME_SYMBOL);
    asset init_quote_balance = asset(0, GAME_SYMBOL);
    asset destroy_balance = asset(0, GAME_SYMBOL);
    asset claim_price = asset(0, GAME_SYMBOL);
    asset hero_reward = asset(0, GAME_SYMBOL);
    account_name hero;

    uint64_t primary_key() const { return gameid; }

    EOSLIB_SERIALIZE(game, (gameid)(status)(counter)(init_max)(total_burn)(total_alive)(total_reserved)(total_lose)(start_time)(end_time)(quote_balance)(init_quote_balance)(destroy_balance)(claim_price)(hero_reward)(hero))
  };

  typedef eosio::multi_index<N(game), game> game_index;
  game_index _games;

  // @abi table bonus i64
  struct bonus
  {
    uint64_t count;                       // 第几次
    uint64_t gameid;                      // 哪一局
    account_name owner;                   // 获奖者
    asset reward = asset(0, GAME_SYMBOL); // 获奖金额
    uint64_t lose_amount;                 // 游离智子数

    uint64_t primary_key() const { return count; }

    EOSLIB_SERIALIZE(bonus, (count)(gameid)(owner)(reward)(lose_amount))
  };

  typedef eosio::multi_index<N(bonus), bonus> bonus_index;

  // @abi table player i64
  struct player
  {
    account_name player_account;
    int64_t join_time = current_time();

    uint64_t primary_key() const { return player_account; }

    EOSLIB_SERIALIZE(player, (player_account)(join_time))
  };

  typedef eosio::multi_index<N(player), player> player_index;

  // @abi table userinfo i64
  struct userinfo
  {
    uint64_t gameid;

    account_name owner;
    int64_t hodl;                              // 持有智子数量
    int64_t claim_status;                      // 见证奖领取状态
    int64_t action_count;                      // 累计操作次数
    int64_t last_action_time = current_time(); // 上一次操作时间
    asset fee_amount = asset(0, GAME_SYMBOL);  // 累计手续费
    asset in = asset(0, GAME_SYMBOL);          // 累计收入
    asset out = asset(0, GAME_SYMBOL);         // 累计支出

    uint64_t primary_key() const { return gameid; }

    EOSLIB_SERIALIZE(userinfo, (gameid)(owner)(hodl)(claim_status)(action_count)(last_action_time)(fee_amount)(in)(out))
  };

  typedef eosio::multi_index<N(userinfo), userinfo> user_resources_table;

  /**
    *  Uses Bancor math to create a 50/50 relay between two asset types. The state of the
    *  bancor exchange is entirely contained within this struct. There are no external
    *  side effects associated with using this API.
    *  Love BM. Love Bancor.
    */
  struct exchange_state
  {
    uint64_t gameid;

    asset supply;

    struct connector
    {
      asset balance;
      double weight = .5;

      EOSLIB_SERIALIZE(connector, (balance)(weight))
    };

    connector base;
    connector quote;

    uint64_t primary_key() const { return gameid; }

    asset convert_to_exchange(connector &c, asset in)
    {

      real_type R(supply.amount);
      real_type C(c.balance.amount + in.amount);
      real_type F(c.weight / 1000.0);
      real_type T(in.amount);
      real_type ONE(1.0);

      real_type E = -R * (ONE - pow(ONE + T / C, F));
      //print( "E: ", E, "\n");
      int64_t issued = int64_t(E);

      supply.amount += issued;
      c.balance.amount += in.amount;

      return asset(issued, supply.symbol);
    }

    asset convert_from_exchange(connector &c, asset in)
    {
      eosio_assert(in.symbol == supply.symbol, "unexpected asset symbol input");

      real_type R(supply.amount - in.amount);
      real_type C(c.balance.amount);
      real_type F(1000.0 / c.weight);
      real_type E(in.amount);
      real_type ONE(1.0);

      // potentially more accurate:
      // The functions std::expm1 and std::log1p are useful for financial calculations, for example,
      // when calculating small daily interest rates: (1+x)n
      // -1 can be expressed as std::expm1(n * std::log1p(x)).
      // real_type T = C * std::expm1( F * std::log1p(E/R) );

      real_type T = C * (pow(ONE + E / R, F) - ONE);
      //print( "T: ", T, "\n");
      int64_t out = int64_t(T);

      supply.amount -= in.amount;
      c.balance.amount -= out;

      return asset(out, c.balance.symbol);
    }

    asset convert(asset from, symbol_type to)
    {
      auto sell_symbol = from.symbol;
      auto ex_symbol = supply.symbol;
      auto base_symbol = base.balance.symbol;
      auto quote_symbol = quote.balance.symbol;

      if (sell_symbol != ex_symbol)
      {
        if (sell_symbol == base_symbol)
        {
          from = convert_to_exchange(base, from);
        }
        else if (sell_symbol == quote_symbol)
        {
          from = convert_to_exchange(quote, from);
        }
        else
        {
          eosio_assert(false, "invalid sell");
        }
      }
      else
      {
        if (to == base_symbol)
        {
          from = convert_from_exchange(base, from);
        }
        else if (to == quote_symbol)
        {
          from = convert_from_exchange(quote, from);
        }
        else
        {
          eosio_assert(false, "invalid conversion");
        }
      }

      if (to != from.symbol)
        return convert(from, to);

      return from;
    }

    EOSLIB_SERIALIZE(exchange_state, (supply)(base)(quote))
  };

  typedef eosio::multi_index<N(market), exchange_state> market;
  market _market;
};

#define EOSIO_ABI_PRO(TYPE, MEMBERS)                                                                                                              \
  extern "C" {                                                                                                                                    \
  void apply(uint64_t receiver, uint64_t code, uint64_t action)                                                                                   \
  {                                                                                                                                               \
    auto self = receiver;                                                                                                                         \
    if (action == N(onerror))                                                                                                                     \
    {                                                                                                                                             \
      eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account");                                        \
    }                                                                                                                                             \
    if ((code == TOKEN_CONTRACT && action == N(transfer)) || (code == self && (action == N(sell) || action == N(destroy) || action == N(claim)))) \
    {                                                                                                                                             \
      TYPE thiscontract(self);                                                                                                                    \
      switch (action)                                                                                                                             \
      {                                                                                                                                           \
        EOSIO_API(TYPE, MEMBERS)                                                                                                                  \
      }                                                                                                                                           \
    }                                                                                                                                             \
  }                                                                                                                                               \
  }

EOSIO_ABI_PRO(itegame, (transfer)(sell)(destroy)(claim))