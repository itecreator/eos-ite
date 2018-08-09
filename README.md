
ITE 游戏智能合约源代码
----

秉承去中心化原则，我们将代码完全开源。并且主动放弃了对于智能合约的任何操作权限。

开发团队对合约没有任何控制权限。

合约代码无任何后门方法。

经过了多轮的严密内测和专业的安全审计。

进行过各种可预见的边界测试，确保没有让黑客有机可乘的代码漏洞。

有发现任何问题，欢迎提交给我们。

**代码完全开源，并且经过了 eospark 的合约[哈希认证](https://eospark.com/MainNet/contract/itedeathstar)。** 


## ITE 游戏相关信息

* 游戏官网: [http://ite.zone](http://ite.zone)
* 合约账号: [itedeathstar](https://eosflare.io/account/itedeathstar)
* 权限移交: [itedeathstar](https://eospark.com/MainNet/Account/itedeathstar)
* 哈希认证: [itedeathstar](https://eospark.com/MainNet/contract/itedeathstar)
* 源代码: [github](https://github.com/itecreator/eos-ite)
* 交流群:  [discordapp](https://discord.gg/er4JYRP)
* 邮件:  [itesourcecode@163.com](itesourcecode@163.com)

## 合约权限的检查方式

```
$ cleos get account itedeathstar
permissions:
     owner     1:    1 wangruixiwww@active,
        active     1:    1 itedeathstar@eosio.code,
memory:
...
```

通过 `get account itedeathstar` 命令可以查看合约账户权限。

或者查看 [eospark](https://eospark.com/MainNet/account/itedeathstar)。

active 权限为合约代码本身，唯一作用，是让合约本身有权限把自己内部的EOS转账给玩家。

~~owner 权限已经设置成 “黑洞” 公钥: **EOS1111111111111111111111111111111114T1Anm** 。 别说BP和BM，地球上已经没有人能更改这个合约了。~~

上一轮被高级“能量猎人”攻击时，我们束手无策。因此本期我们选择一个折衷的方案。不再是一开始，就把 owner 权限移交给无可挽回的 “黑洞”公钥。而是移交给值得大家信任的账号：wangruixiwww 。之后再择机移交“黑洞”。

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

// 获取每局游戏的的玩家列表
cleos get table iteblackhole {gameid} player 

```

## 游戏规则

* 游戏开始时间为 **2018-08-11 15:00:00** ，时间直接写入智能合约。任何人无法在此时间之前，向合约进行转账EOS买入操作。从技术角度，确保所有人真正处于同一个起跑线。

* 合约内自带所有玩家账号的收支统计，操作次数统计，手续费支出。全部数据透明。

*  智子回收价： 智子实时价格的90%

* 终极大奖： 获得订单操作额度的10倍奖励，不超过奖池10% 。最大 5000 EOS。 

* 能量泄漏的奖励，挂钩该幸运玩家的历史总手续费。最大不超过历史总手续费。

* 能量泄漏所还原的智子，将直接进入游离状态（等同于回收）。于是，每一次能量泄漏，泄漏的部分智子，将会降低“工程进度”的同时 ，增加 “毁灭进度” 。

* 能量泄漏计数器，加入门槛。需要单笔操作金额大于等于 5 EOS。才会增加计数器。小额刷单，对计数器无效。

* 大单限制: 每次充能、回收的智子数量。不超过当前总智子数的 1% 

* 冷却时间限制: 每一次进行充能、还原、回收。账号将进入15秒的操作冷却时间。需等15秒之后，才能执行下一次操作。

## 操作说明

* 充能:使用转换引擎将 EOS 合成为智子，推进工程进度。 

* 还原:使用转换引擎将智子转化回 EOS，降低工程进度。 

* 回收:调用转换引擎回收程序将已充能智子程序按照 90%的价格回收 EOS，等量释放游离态元素，推进毁灭进度。

* 单笔数量上限: 每次充能、还原、回收的智子数量。不超过当前总智子数的 1%。 （前期大概100 EOS, 随智子涨价而提高额度）。(防止前期大户一次性垄断买入。防止后期大户无脑绝杀，游戏猝死。)

* 冷却时间限制: 每一次进行充能、还原、回收。账号将进入 15 秒的操作冷却时间。


## 玩家角色

* 参与人:任何参与者在该轮游戏结束后都将获得⻅证者奖金。

* 建造人:不仅将在 ITE 之⻔上镌刻下他的名字，还将获得巨额成就奖励。 

* 毁灭者:虽然摧毁了宏伟工程，但也或许暂时保护了人类文明，也将获得巨额成就奖励。 

* 材料商:在“ITE 之⻔”的建设过程中，大多数人是没有立场的。充能或者还原，只要有利可图。 

* 能量猎人:发现了能量库的设计缺陷，定点蹲守能量泄露。以此为生，以此为乐。

## 名词解释

* 能量库:由星际开发总署建造，存有所有玩家注入的 EOS，与转换引擎相连，近期被 发现存在设计缺陷经常导致能量泄露。

* 工程进度:用于标示“ITE 之⻔”的建造进度，   

* 毁灭进度:用于标示“ITE 之⻔”的不稳定状态，  

* 智子: 建造人工虫洞的基础材料，可以使用转换引擎实现与 EOS 的双向转换。 

* 转换引擎:基于 Bancor 算法实现的自动化交易引擎，主要用于完成 EOS/智子定价。掌握不完整的高阶科技，每次使用时需要支付 0.5%的费用，也容易造成能量泄露。在进行任何 操作时都会有 1%失控游离，近期毁灭派发现可以调用内置回收程序等量释放游离态元素，以 上都会推进毁灭进度。

* 能量泄露: 由于能量库的设计缺陷，转换引擎每运行 1000 次。便奖励该次操作的触发者 ，获得奖励后该玩家的总手续费清零重新开始累计。能量泄漏所还原的智子，将直接进入游离状态（等同于回收）。所以，每一次能量泄漏，将会降低“工程进度”的同时 ，增加 “毁灭进度” 。

* 能量泄漏计数器： 需要单笔操作金额大于等于 5 EOS。才会增加计数器。小额刷单，对计数器无效。

* 终极大奖:触发进度达成的用户，将会获得触发条件达成的单笔 EOS 注入金额的 10 倍奖励，不超过能量库的 10%，最大 5000EOS。

* ⻅证者奖金: 游戏结局后，每个持有智子的玩家都能获得的奖励。计算方式为： 个人智子持仓 * (能量库 - 终极大奖) / (未回收智子 + 能量泄漏智子）

## 游戏结局

Good Ending: 人类达成共识。工程进度完成。建成 ITE 星域之门。开启银河系之外的新征程。 

Bad Ending:  人类始终无法达成共识。在内部斗争中，珍贵的智子被销毁过半。ITE 之门建设失败。 

## 全部数学公式公开

> In math we trust.

从 game table 和 global table 中获取数据以后。

根据以下数学公式，任何人，可以自主推导出整个游戏当中，任意阶段的数据。所有数据均由算法决定，无任何人为干涉。

```

实时智子价格: current_price = (quote_balance - destroy_balance) / (init_max - total_reserved)

// bancor算法中, 每一笔大单都会拆成无数小单。所以以下价格预测。仅供参考。小单的情况下相对, 大单会存在比较大的偏差。
实时智子充能价格预测:  eos_amount * 0.995 / current_price

实时智子还原价格预测:  current_price * sell_amount * 0.995

当前智子回收价格预测:  current_price / 100 * burn_price_ratio

终极大奖预测: end_prize <= (quote_balance - init_quote_balance) * 10% ,  end_prize <= 5000 EOS

// 具体数值取决于最后大奖, 因此无法准确预估
见证者奖预测: claim_price = (quote_balance - init_quote_balance - end_prize) / (total_reserved + total_lose)

建造结局进度: ( total_reserved ) / ( total_alive / 100 * bad_ending_ratio ) 

毁灭结局进度: ( total_burn ) / ( init_max  / 100 * good_ending_ratio )

// 和每个用户的个人历史总手续费相关。不超过个人历史总手续费。
能量泄漏最大奖励: userinfo.fee_amount 

单次操作最大智子数: total_alive / 100 * max_operate_amount_ratio

```


