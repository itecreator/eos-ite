
ITE 游戏智能合约源代码
----

秉承去中心化原则，我们选择将代码开源。并且主动放弃了对于智能合约的任何操作权限。

开发团队对合约没有任何控制权限。

合约代码无任何后门方法。

经过了多轮的严密内测和专业的安全审计。

进行过各种可预见的边界测试，确保没有让黑客有机可乘的代码漏洞。

有发现任何问题，欢迎提交给我们。

( 代码仅供学习交流，为了防止不法分子直接利用本源代码，去进行诈骗。我们将如何编译代码的部分隐藏，并在核心业务逻辑不变的情况下, 主动藏了些的小BUG, 以防代码被直接拿去恶意使用。)

## ITE 游戏相关信息

* 游戏官网: [http://ite.zone](http://ite.zone)
* 合约账号: [iteblackhole](https://eosflare.io/account/iteblackhole)
* 源代码: [github](https://github.com/itecreator/eos-ite)
* 交流群:  [discordapp](https://discord.gg/er4JYRP)
* 邮件:  [itesourcecode@163.com](itesourcecode@163.com)

## 合约权限的检查方式

```
$ cleos get account iteblackhole
permissions:
     owner     1:    1 EOS1111111111111111111111111111111114T1Anm
        active     1:    1 iteblackhole@eosio.code,
memory:
...
```

通过 `get account iteblackhole` 命令可以查看合约账户权限。

或者查看 [eospark](https://eospark.com/MainNet/account/iteblackhole).

owner 权限已经设置成 “黑洞” 公钥: **EOS1111111111111111111111111111111114T1Anm** 。 别说BP和BM，地球上已经没有人能更改这个合约了。

active 权限为合约代码本身，唯一作用，是让合约本身有权限把自己内部的EOS转账给玩家。

开发者已经对合约没有任何控制权限。

比起 Fomo3D 预留的两个未开源的合约代码。

**ITE，这是一个真正意义上不会跑路，无法修改，没有后门的区块链游戏。**


## 全部数据上链

全部数据上链。所有数据透明，任何第三方，都可以通过以下数据接口，获取游戏数据。或根据合约ABI描述的智能合约接口，对接本游戏。

```
// 获取玩家数据, 其中 account_name 替换为要查询的EOS用户名
cleos get table iteblackhole {account_name} userinfo

// 获取游戏数据
cleos get table iteblackhole iteblackhole game 

// 获取全局数据
cleos get table iteblackhole iteblackhole global 

// 获取每局游戏的阶段奖励记录 gameid 替换为要查询的游戏编号
cleos get table iteblackhole {gameid} bonus 

```

## 全部数学公式公开

> In math we trust.

从 game table 和 global table 中获取数据以后。

根据以下数学公式，任何人，可以自主推导出整个游戏当中，任意阶段的数据。这是这个游戏所展现的数学之美：所有数据均由算法决定，不存在任何人为干涉。

```
当前智子实时价格: ( quote_balance ) / ( total_alive - total_reserved )

当前智子回收价格:  ( quote_balance ) / ( total_alive - total_reserved ) / 100 * burn_price_ratio

建造结局进度:  ( total_reserved ) / ( total_alive / 100 * bad_ending_ratio ) 

毁灭结局进度: ( total_burn ) / ( init_max  / 100 * good_ending_ratio )

能量泄漏最大奖励: ( quote_balance - init_quote_balance ) / 100 * ( end_prize_ratio / 10 )

终极大奖最大奖励: ( quote_balance - init_quote_balance ) / 100 * end_prize_ratio

见证者最大奖励（具体数值取决于最后大奖, 因此无法准确预估）: ( quote_balance - init_quote_balance - final_rewark ) / total_reserved * userinfo.hodl

```


