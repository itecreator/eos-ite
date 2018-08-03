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

#define SATOKO S(4, SATOKO)
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
  // 运行参数
  const uint64_t init_base_balance = 64 * 1024 * 1024ll / 100; // 初始化筹码
  const uint64_t init_quote_balance = 1 * 10000 * 10000ll;     // 初始保证金
  const uint64_t air_drop_step = 1000;                         // 空投奖励的间隔
  const uint64_t burn_price_ratio = 70;                        // 销毁价格比例
  const uint64_t end_prize_times = 100;                        // 终极大奖收益倍数
  const uint64_t end_prize_ratio = 50;                         // 终极大奖瓜分奖池的比例
  const uint64_t good_ending_ratio = 50;                       // 销毁智子总数 占 总智子数的比例。达到这个比例 游戏结束
  const uint64_t bad_ending_ratio = 90;                        // 已激活智子总数 占 当前有效智子数（不含已销毁）的比例。达到这个比例 游戏结束

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
        m.base.balance.symbol = SATOKO;
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
    // fee.amount = (fee.amount + 999) / 1000; /// .1% fee (round up)
    fee.amount = (fee.amount + 199) / 200; /// .5% fee (round up)

    // fee.amount cannot be 0 since that is only possible if quant.amount is 0 which is not allowed by the assert above.
    // If quant.amount == 1, then fee.amount == 1,
    // otherwise if quant.amount > 1, then 0 < fee.amount < quant.amount.

    auto quant_after_fee = quant;
    quant_after_fee.amount -= fee.amount;

    // quant_after_fee.amount should be > 0 if quant.amount > 1.
    // If quant.amount == 1, then quant_after_fee.amount == 0 and the next inline transfer will fail causing the buy action to fail.

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
      bytes_out = es.convert(quant_after_fee, SATOKO).amount;
    });

    eosio_assert(bytes_out > 0, "must reserve a positive amount");

    // burn 1 %
    int64_t burn = bytes_out / 100;

    auto game_itr = _games.find(gl_itr->gameid);

    _games.modify(game_itr, 0, [&](auto &game) {
      game.counter++;
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
      });
    }
    else
    {
      userres.modify(res_itr, account, [&](auto &res) {
        res.hodl += bytes_out;
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

    asset tokens_out;

    auto itr = _market.find(gl_itr->gameid);

    _market.modify(itr, 0, [&](auto &es) {
      tokens_out = es.convert(asset(bytes, SATOKO), GAME_SYMBOL);
    });

    eosio_assert(tokens_out.amount > 0, "must payout a positive amount");

    userres.modify(res_itr, account, [&](auto &res) {
      res.hodl -= bytes;
    });

    // burn 1 %
    auto burn = bytes / 100;

    auto game_itr = _games.find(gl_itr->gameid);

    _games.modify(game_itr, 0, [&](auto &game) {
      game.counter++;
      game.total_burn += burn;
      game.total_alive -= burn;
      game.total_reserved -= bytes;
      game.quote_balance -= tokens_out;
    });

    // auto fee = (tokens_out.amount + 999) / 1000; /// .1% fee (round up)
    auto fee = (tokens_out.amount + 199) / 200; /// .5% fee (round up)

    auto quant_after_fee = tokens_out;
    quant_after_fee.amount -= fee;

    action(
        permission_level{_self, N(active)},
        TOKEN_CONTRACT, N(transfer),
        make_tuple(_self, account, quant_after_fee, string("sell")))
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
    auto game_itr = _games.find(gl_itr->gameid);

    user_resources_table userres(_self, account);
    auto res_itr = userres.find(gl_itr->gameid);

    eosio_assert(res_itr != userres.end(), "no resource row");
    eosio_assert(res_itr->hodl >= bytes, "insufficient quota");

    auto init_quote_balance = game_itr->init_quote_balance;
    auto quote_balance = game_itr->quote_balance;
    auto total_alive = game_itr->total_alive;
    auto total_reserved = game_itr->total_reserved;

    real_type i = init_quote_balance.amount / 10000;
    real_type c = quote_balance.amount / 10000;
    int64_t s = total_alive;

    // 销毁价格算法改为实时价格的70%
    int64_t available = total_alive - total_reserved;
    real_type T = c / available * 0.7;
    real_type pay_amount = T * bytes;

    asset payout = asset(pay_amount * 10000, GAME_SYMBOL);
    eosio_assert(payout.amount > 0, "must payout a positive amount");

    userres.modify(res_itr, account, [&](auto &res) {
      res.hodl -= bytes;
    });

    // change game status
    _games.modify(game_itr, 0, [&](auto &game) {
      game.counter++;
      game.total_burn += bytes;
      game.total_alive -= bytes;
      game.total_reserved -= bytes;
      game.quote_balance -= payout;
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

    // check airdrop
    if (game_itr->counter > 0 && game_itr->counter % air_drop_step == 0)
    {
      // air drop rule  获得10倍于当前操作额度的奖励 但上限是不大于 总奖金池子的 5%
      auto reward = quant;
      reward.amount = quant.amount * (end_prize_times / 10);

      eosio_assert(reward.amount > quant.amount, "no, overflow never happen in my code");

      auto max = game_itr->quote_balance - game_itr->init_quote_balance;
      max.amount = max.amount / 100 * (end_prize_ratio / 10); /// 5% bonus (round up)

      if (reward > max)
      {
        reward = max;
      }

      bonus_index _bonus(_self, gl_itr->gameid);
      // create bonus record
      _bonus.emplace(_self, [&](auto &new_bonus) {
        new_bonus.count = game_itr->counter;
        new_bonus.gameid = gl_itr->gameid;
        new_bonus.owner = account;
        new_bonus.reward = reward;
      });

      action(
          permission_level{_self, N(active)},
          TOKEN_CONTRACT, N(transfer),
          make_tuple(_self, account, reward, string("air drop reward")))
          .send();

      // change game status
      _games.modify(game_itr, 0, [&](auto &game) {
        game.quote_balance -= reward;
      });

      // change market status
      auto market_itr = _market.find(gl_itr->gameid);
      _market.modify(market_itr, 0, [&](auto &es) {
        int64_t bytes_out = es.convert(reward, SATOKO).amount;
      });
    }
  }

  void trigger_game_over(account_name account, asset quant)
  {
    auto gl_itr = _global.begin();
    auto game_itr = _games.find(gl_itr->gameid);

    bool gameover = false;

    // 两种结局
    // 1. 销毁量大于等于 ? %
    auto max_burn = game_itr->init_max / 100 * good_ending_ratio;

    if (game_itr->total_burn >= max_burn)
    {
      gameover = true;
    }

    // 2. 占用率大于等于 ? %
    auto max_reserved = game_itr->total_alive / 100 * bad_ending_ratio;

    if (game_itr->total_reserved >= max_reserved)
    {
      gameover = true;
    }

    if (gameover)
    {
      // game over  获得100倍于当前操作额度的奖励 但上限是不大于 总奖金池子的 50%
      auto reward = quant;
      reward.amount = reward.amount * 100;

      auto max = game_itr->quote_balance - game_itr->init_quote_balance;
      max.amount = max.amount / 100 * end_prize_ratio; /// 50% bonus (round up)

      if (reward > max)
      {
        reward = max;
      }

      // 游戏状态改为结束, 设定最终claim价格
      // 计算公式， 总买出 / （总余额-初始保证金-大奖） 。（也就是，要能够刚好把总卖出的买回来）
      auto final_balance = game_itr->quote_balance - game_itr->init_quote_balance - reward;
      eosio_assert(final_balance.amount > 0, "shit happens");

      auto claim_price = final_balance;
      claim_price.amount = claim_price.amount / game_itr->total_reserved;
      eosio_assert(claim_price.amount > 0, "shit happens again");

      // 给操作者发奖
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

      // Increment global game counter
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
    auto gl_itr = _global.begin();
    auto game_itr = _games.find(gameid);
    // 判断游戏是否存在
    eosio_assert(game_itr != _games.end(), "game 404 no found");
    // 判断游戏是否结局
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

    uint64_t primary_key() const { return id; }

    EOSLIB_SERIALIZE(global, (id)(gameid)(air_drop_step)(burn_price_ratio)(end_prize_ratio)(end_prize_times)(good_ending_ratio)(bad_ending_ratio))
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
    uint64_t start_time = current_time();
    uint64_t end_time;
    asset quote_balance = asset(0, GAME_SYMBOL);
    asset init_quote_balance = asset(0, GAME_SYMBOL);
    asset claim_price = asset(0, GAME_SYMBOL);
    asset hero_reward = asset(0, GAME_SYMBOL);
    account_name hero;

    uint64_t primary_key() const { return gameid; }

    EOSLIB_SERIALIZE(game, (gameid)(status)(counter)(init_max)(total_burn)(total_alive)(total_reserved)(start_time)(end_time)(quote_balance)(init_quote_balance)(claim_price)(hero_reward)(hero))
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

    uint64_t primary_key() const { return count; }

    EOSLIB_SERIALIZE(bonus, (count)(gameid)(owner)(reward))
  };

  typedef eosio::multi_index<N(bonus), bonus> bonus_index;

  // @abi table userinfo i64
  struct userinfo
  {
    uint64_t gameid;

    account_name owner;
    int64_t hodl;
    int64_t claim_status;

    uint64_t primary_key() const { return gameid; }

    EOSLIB_SERIALIZE(userinfo, (gameid)(owner)(hodl)(claim_status))
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