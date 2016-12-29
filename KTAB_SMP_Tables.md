# SQL Logging Group 0 - Information Tables
## `ScenarioDesc(Scenario, Desc, ScenarioId*, RNGSeed)`
    Scenario    Text(512)  the scenario name which is created as a function of the UTC date & time of when the scenario was begun
    Desc        Text(512)  the long description of the scenario is either input from a model .csv file or set to be the same as Scenario if not present
    ScenarioId  Text(32)  the UTC time down to milliseconds of when the scenario was begun, hashed to a 32-character hex string
    RNGSeed     Text(20)   the seed which sets the initial state for the random number generator
This table stores all the scenarios for which the model was run and for which results are stored in this database file.

## `ActorDescription(ScenarioId*, Act_i*, Name, Desc)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Act_i       Integer   id number for the actor; [0, number actors-1]
    Name        Text(25)  the short code of the actor, which is printed in console output and also on the GUI charts
    Desc        Text(256) the longer description of the actor
This table stores all the actors and their constant information.

## `SpatialCapability(ScenarioId*, Turn_t*, Act_i*, Cap)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    Act_i       Integer   Foreign Key into ActorDescription; actor whose capability is recorded; [0,number actors-1]
    Cap         Real    capability for actor Act_i during iteration Turn_t; [0,infty)
Capabilities for all actors are stored in `SpatialCapability`. While capability is currently constant throughout a model run, it could conceivably change over time.  Hence, this is more general to store capabilities per iteration.

## `SpatialSalience(ScenarioId*, Turn_t*, Act_i*, Dim_k*, Sal)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    Act_i       Integer   Foreign Key into ActorDescription; actor whose salience is recorded; [0,number actors-1]
    Dim_k       Integer   dimension number for which the salience is recorded; [0, number dimensions-1]
    Sal         Real    salience for actor Act_i in dimension Dim_k during iteration Turn_t; [0,1]
Actors saliences are all stored in this table.  As with `SpatialCapability`, they are stored by iteration, though they don't currently change as a model progresses.

# SQL Logging Group 1 - Position Tables
## `PosUtil(ScenarioId*, Turn_t*, Est_h*, Act_i*, Pos_j*, Util)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    Est_h       Integer   Foreign Key into ActorDescription; actor from whose view the utilities are recorded; [0,number actors-1]
    Act_i       Integer   Foreign Key into ActorDescription; actor for whom the utility is recorded; [0,number actors-1]
    Pos_j       Integer   Foreign Key into ActorDescription; actor for whose position the utility is recorded; [0,number actors-1]
    Util        Real  Utility to actor Act_i of the position of actor Pos_j, as estimated by actor Est_h; [0,1]
`PosUtil` records, for each iteration, the utility to *every* actor of *every* actors position, from the perspective of *every* actor. For each iteration, there are (number actor)^3 records.

## `PosEquiv(ScenarioId*, Turn_t*, Pos_i*, Eqv_j*)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    Pos_i       Integer   Foreign Key into ActorDescription; actor whose position is on the LHS of the equality position i == position j; [0,number actors-1]
    Eqv_j       Integer   Foreign Key into ActorDescription; actor whose position is on the RHS of the equality position i == position j; [0,number actors-1]
As actors move their positions, some will naturally end up at the same position - within a certain tolerance, which is currently set to 0.001. This table records in each iteration all pairs of same-position actors. This also includes the trivial equality of `position i == position j`.

## `PosProb(ScenarioId*, Turn_t*, Est_h*, Pos_i*, Prob)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    Est_h       Integer   Foreign Key into ActorDescription; the actor for who the probability is computed; [0,number actors-1]
    Pos_i       Integer   Foreign Key into ActorDescription; actor for whose position the probability is computed; [0,number actors-1]
    Prob        Real      the probability for actor Est_h associated with the position of actor Pos_i; [0,1]
This table is supposed to record, for each actor in each turn, the probability distribution function over all *unique* positions, according to the equivalence relations stored in PosEquiv.

## `PosVote(ScenarioId*, Turn_t*, Est_h*, Voter_k*, Pos_i*, Pos_j*, Vote)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    Est_h       Integer   Foreign Key into ActorDescription; actor who estimates how actor Voter_k will vote; [0,number actors-1]
    Voter_k     Integer   Foreign Key into ActorDescription; actor who votes between positions held by actors Pos_i and Pos_j; [0,number actors-1]
    Pos_i       Integer   Foreign Key into ActorDescription; one actor on whose position is being voted; [0,number actors-1]
    Pos_j       Integer   Foreign Key into ActorDescription; other actor on whose position is being voted; [0,number actors-1]
    Vote        Real  vote of actor Voter_k between the positions held by actors Pos_i and Pos_j, estimated by actor Est_h; (-infty, infty) is this correct?
`PosVote` records in each iteration with how every actor votes for all pairs of positions, estimated by each actor.

# SQL Logging Group 2 - Challenge Tables
## `UtilChlg(ScenarioId*, Turn_t*, Est_h*, Aff_k*, Init_i*, Recvr_j*, Util_SQ, Util_Vict, Util_Cntst, Util_Chlg)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    Est_h       Integer   Foreign Key into ActorDescription; actor from whose view the utilities are recorded; [0,number actors-1]
    Aff_k       Integer   Foreign Key into ActorDescription; actor in whose benefit utilities are recorded; [0,number actors-1]
    Init_i      Integer   Foreign Key into ActorDescription; challenging actor; [0,number actors-1]
    Recvr_j     Integer   Foreign Key into ActorDescription; challenged actor; [0,number actors-1]
    Util_SQ     Real  expected utility to Aff_k of the status quo vis-a-vis actors Init_i and Recvr_j; [0,2]
    Util_Vict   Real  expected utility to Aff_k if Recvr_j were to take the position of Init_i, this will always be 2.0 when Aff_k = Init_i; [0,2]
    Util_Cntst  Real  coalition-weighted average of the utilities to Aff_k of Init_i convincing Recvr_j to take his position and Recvr_j convincing Init_i to take his position; [0,2]
    Util_Chlg   Real  salience-weighted expectation of the utilities to Aff_k of Util_Vict vs Util_Cntst; [0,2]
The table `UtilChlg` stores some utility-related calculations for position challenges. Specifically, the utilities are computed as "Actor `Est_h`'s estimate of the utility to actor `Aff_k` from actor `Init_i` challenging actor `Recvr_j` to take `Init_i`'s position. **Note that the "expected utilities" stored in this table are all actually 2x expected utilities. Due to how they are used, there's no arithmetical difference between using the U's and 2U's, but it is confusing. This will likely be fixed at some point.**

## `TP_Prob_Vict_Loss(ScenarioId*, Turn_t*, Est_h*, Init_i*, ThrdP_k*,Rcvr_j*, Prob, Util_V, Util_L)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    Est_h       Integer   Foreign Key into ActorDescription; actor from whose view the utilities are recorded; [0,number actors-1]
    Init_i      Integer   Foreign Key into ActorDescription; challenging actor; [0,number actors-1]
    ThrdP_k     Integer   Foreign Key into ActorDescription; actor in whose benefit utilities are recorded; [0,number actors-1]
    Recvr_j     Integer   Foreign Key into ActorDescription; challenged actor; [0,number actors-1]
    Prob        Real      Probability of actor ThrdP_k supporting actor Init_i over actor Recvr_j, computed as the ratio of influences; [0,1]
    Util_V      Real  Utility to actor ThrdP_k if he supports actor Init_i over actor Recvr_j; [0,3]
    Util_L      Real  Utility to actor ThrdP_k if he supports actor Recvr_j over actor Init_i; [0,3]
The data stored in `TP_Prob_Vect_Loss` is from the same part of the model as  `UtilChlg`, except the data is related to computing which actor the third party actor `ThrdP_k` should actually support. Again, the utilities are computed as "Actor `Est_h`'s estimate of the utility to actor `ThrdP_k` from actor `Init_i` challenging actor `Recvr_j` to take `Init_i`'s position. **Note that the "expected utilities" stored in this table are all actually 3x expected utilities. Due to how they are used, there's no arithmetical difference between using the U's and 3U's, but it is confusing. This will likely be fixed at some point.**

## `ProbVict(ScenarioId*, Turn_t*, Est_h*, Init_i*,Rcvr_j*, Prob)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    Est_h       Integer   Foreign Key into ActorDescription; actor from whose view the utilities are recorded; [0,number actors-1]
    Init_i      Integer   Foreign Key into ActorDescription; challenging actor; [0,number actors-1]
    ThrdP_k     Integer   Foreign Key into ActorDescription; actor in whose benefit utilities are recorded; [0,number actors-1]
    Recvr_j     Integer   Foreign Key into ActorDescription; challenged actor; [0,number actors-1]
    Prob        Real      Probability of actor Init_i winning the bargain over actor Recvr_j, computed as the ratio of the coalition supporting Init_i over the sum of the coalitions; [0,1]
This table stores the culmination of all the calculations recorded in `UtilChlg` and `TP_Prob_Vict_Loss`. The resulting `Prob` is used in the model to guide bargain resolution.

# SQL Logging Group 3 - Bargain Resolution Tables
## `Bargn(ScenarioId*, Turn_t*, BargnId*, Init_Act_i, Recd_Act_j, Value, Init_Prob, Init_Seld, Recd_Prob, Recd_seld)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    BargnId     Integer   bargain ID number from the BargainSMP.getID() method, this is automatically incremented each time a new bargain object is created; [1000,infty)
    Init_Act_i  Integer   Foreign Key into ActorDescription; actor that initiated the bargain; [0,number actors-1]
    Recd_Act_j  Integer   Foreign Key into ActorDescription; actor that received the bargain proposal; [0,number actors-1]
    Value       Real      increase in utility the initiating actor expects from the bargain; [0,1]
    Init_Prob   Real      probability that the bargain will be accepted when resolved inside the initiating actor's queue; [0,1]
    Init_Seld   Boolean   indicates whether or not the bargain was accepted when resolved inside the initiating actor's queue; either 0 (False) or 1 (True)
    Recd_Prob   Real      probability that the bargain will be accepted when resolved inside the receiving actor's queue; [0,1]
    Recd_Seld   Boolean   indicates whether or not the bargain was accepted when resolved inside the receiving actor's queue; either 0 (False) or 1 (True)
The `Bargn` table records bargains initiated by each actor during the bargain and resolution phase in each iteration of the model.  Each bargain consists of a new position for both the initiating actor and receiving actor, and every bargain is evaluated in both actors' queues.  Hence, this table also records the probability of being selected, as well as actual selection result, from both queues.

## `BargnCoord(ScenarioId*, Turn_t*, BargnId*, Dim_k*, Init_Coord, Recd_Coord)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    BargnId     Integer   Foreign Key into Bargn, bargain ID number from the BargainSMP.getID() method; [1000,infty)
    Dim_k       Integer   dimension number for which the proposed coordinates are recorded; [0, number dimensions-1]
    Init_Coord  Real      the proposed coordinate in dimension Dim_k for the initiating actor according to this bargain; [0,1]
    Recd_Coord  Real      the proposed coordinate in dimension Dim_k for the receiving actor according to this bargain; [0,1]
Every bargain consists of an initiating actor proposing to a receiving actor a new set of positions for both of them.  These positions are coded as coordinates in the number-of-dimensions space.  These coordinates are recorded in this table, with Dim_k looping through all dimensions for each `BargnId`.  The coordinates `Init_Coord` are the proposed position for actor `Bargn.Init_Act_i`, and `Recd_Coord` are for actor `Bargn.Recd_Act_i`

## `BargnUtil(ScenarioId*, Turn_t*, BargnId*, Act_k*, Util*)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    BargnId     Integer   Foreign Key into Bargn, bargain ID number from the BargainSMP.getID() method; [1000,infty)
    Act_k       Integer   Foreign Key into ActorDescription; actor that is evaluating the utility to himself of the bargain; [0,number actors-1]
    Util        Real      the utility actor Act_k expects from this bargain if it were to be selected; [0,1]
Every proposed bargain is evaluated in at least two queues: the initiating actor's queue, and the receiving actor's queue.  Status quo bargains are evaluated in every actor's queue.  In each queue, every actor computes the utility they expect from each bargain; these expected utilities are stored in this table.  Note that the utility for actor `Act_i` for bargain `BargainId` is actually computed twice, since each bargain is present in two queues.  Since the utility of a bargain is independent of the queue in which it's evaluated, the resulting utility is the same both times.  However, it could be more overhead to check if it was already recorded, or set a unique key enforcing referential integrity to prevent duplicated records, so we just allow duplicated records.

## `BargnVote(ScenarioId*, Turn_t*, BargnId_i*,  BargnId_j*, Act_k*, Vote)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    BargnId_i     Integer   Foreign Key into Bargn, bargain ID number from the BargainSMP.getID() method, first bargain in pair which are being evaluated; [1000,infty)
    BargnId_j     Integer   Foreign Key into Bargn, bargain ID number from the BargainSMP.getID() method, second bargain in pair which are being evaluated; [1000,infty)
    Act_k       Integer   Foreign Key into ActorDescription; actor who is voting between bargains Bargn_i vs. Bargn_j in the queue for actor Queue_n; [0, number actors-1]
    Vote        Real      the vote of actor Act_k between bargains Bargn_i and Bargn_j; (-infty,infty)
Every proposed bargain is evaluated in two queues: the initiating actor's queue, and the receiving actor's queue.  Status quo bargains are evaluated in every actor's queue. In each queue, all actors vote on **all pairs of all bargains**.  Hence, if there are three bargains in a queue, named *A*, *B*, *C*, all actors will compute the following votes: *A vs B*, *A vs C*, *B vs C*, *B vs A*, *C vs A*, and *C vs B*.  These votes are stored in this table, looping over all queues, all pairs of bargains (in each queue), and all actors.  Calculations with these votes are then used to compute the coalition strengths, which are then used to compute the select probabilities (and results) which are stored in `Bargn.Init_Prob`, `Bargn.Recd_Prob`, `Bargn.Init_Seld`, and `Bargn.Recd_Seld`.

# SQL Logging Group 4 - Position History Table
## `VectorPosition(ScenarioId*, Turn_t*, Act_i*, Dim_k*, Coord)`
    ScenarioId  Text(32)  Foreign Key into ScenarioDesc; id number for the scenario
    Turn_t      Integer   iteration number, begins with 0 and increments by 1 each iteration; [0,infty)
    Act_i       Integer   Foreign Key into ActorDescription; actor whose position is recorded; [0,number actors-1]
    Dim_k       Integer   dimension number for which the position is recorded; [0, number dimensions-1]
    Pos_Coord       Real      position for actor Act_i in dimension Dim_k at the end of iteration Turn_t; [0,1]
    Idl_Coord       Real      ideal position for actor Act_i in dimension Dim_k at the end of iteration Turn_t; [0,1]
At the end of every iteration of the model, each actor has the potential to have shifted his actual position. This table records the history of these state changes.


* fields indicates the primary key for each table