Euro1943
Version 1.1b
Greg Kennedy
-----

Join the Axis or the Allies in this multiplayer team-based action game. Pick
up weapons to help you fight enemies and take over strategic capture points on
the map. Climb into a tank, gunboat, or fighter plane and support the infantry
on the ground. Or hop into the HQ and spend your team's funds on weapons and
vehicles for the players to use.

This game is built on the SDL libraries and uses the SDL_image, SDL_net and
SDL_mixer addon libraries.  The required license notice is included at the
bottom of this readme.

Please see LICENSE.TXT for more information about who can use this software,
what you can do with it, etc.  The long and short of it is that this is
freeware, source code is not available (sorry, it's a wreck), and I'm not
responsible for what happens to your computer when using this software.

Elements
----
The 4eV contest (www.gamedev.net) requires four elements present in the game.
Here is a list of the elements, and a description of how my game meets all four.

Europe:  The game takes place in Europe around the end of World War 2.  All
     locations are assumed to be in Europe (although there are no real-world
     likenesses of any maps in the game).

Economy:  The role of the Commander in multiplayer satisfies this role.  There
     are money sources from the capture points, money losses from dying players
     and constant upkeep, and a system that allows the Commander to buy items
     to benefit his team.
     This system is generally transparent to the non-Commander, however.

Emotion:  The storyline of the single player game attempts to convey the
     emotions felt by the player's character.  I am not a novelist so it is not
     as powerful as I would like; nevertheless, the player frequently announces
     in cutscenes lines such as "I certainly feel PROUD", "I'm horrified", etc.

Emblem:  I used the American Flag as the emblem in the game.  It is not widely
     featured (and in fact does not appear in the multiplayer at all), but it
     does appear in nearly every single-player cutscene.  I chose it to
     represent a set of "good virtues" in the beginning, and as the storyline
     progresses, the player character questions what it really stands for.

Controls
----
WASD Arrows     Dvorak  Action
W/S  Up/Down    ,/O     Accelerate forward / Accelerate backwards
A/E  Left/Right A/D     Rotate counter-clockwise / Rotate clockwise
C               C       Global chat
T               T       Team chat

Left mouse button         Fire primary weapon
Right mouse button        Fire secondary weapon / Use sniper zoom
Middle mouse button/Enter Enter/exit vehicle or HQ
Mouse wheel/PgUp/PgDn     Swap weapons

The game can be run in Windowed mode by passing a -w on the command line.

Objective
----
As a player, your objective is to take over and hold the three capture points on
the map.  Every capture point you take generates more additional income for your
team and less for the opposing team.  Additionally, killing enemy soldiers costs
your opponents money.

Players can move around and fire using the mouse and keyboard.  When you enter
the game you are armed with only a pistol.  Your commander may drop additional
weapons on the ground which can give you an edge in combat.  The pistol has
unlimited ammo, but your other weapons will rapidly consume your bullets, and
you need to find ammo kits either dropped by a commander or handed out by a
teammate with a backpack.

Use the middle mouse button or Enter key to get into a vehicle.  Depending on
how many other players are also in the vehicle, you may or may not be able to
drive it using the keyboard.  If you are controlling a turret, you may move and
fire it independently with the mouse and mouse buttons.  Note that a player in
a vehicle cannot capture a point - they must exit the vehicle first and stand
on it.

One player at a time may occupy their team's HQ.  The HQ player is shown a top
view of the map, the current amount of cash both teams posess, and a menu of
possible purchases the player may make for their team.  The Commander must
balance spending the team's money so he does not endanger their chances of
winning the round while still providing appropriate munitions to the team.
Additionally he must learn how not to place vehicles so the boats are not stuck
on land, etc.

Finances
----
Each team spends $2 every second to pay the upkeep of the war.
Each capture point generates $4 every second for the team that owns it.
Each player spawn costs their team $15.
The Commander may spend the rest of his team's money as he sees fit.
The round is over when a team has less than $1 remaining.

Server Control
----
The server is controlled through a server.ini file located in the server/ directory.
If it does not exist when loaded, a default one will be created.  The format
of the file is:

overserver location
overserver port
map
map
map
...

This allows servers to run a "map rotation" - each map in the list is played
sequentially for five rounds, and then the next map is loaded.
If there are problems starting the server, try to delete the server.ini as
a first step - a new default ini will be created instead.
Change the overserver location and port at your own risk: if you have a
different OverServer you'd rather use (say, for tournament usage) then these
can be modified.  The default settings are usually fine for most users.

Map Editor
----
There is a map editor available (Windows only) - it is a Visual Basic 6 app.
It's not very polished but it is functional enough to create maps.
A few notes on using it:
* Choose a tile from the palette, then click to paint it on the map.
* The text boxes on the right side define up to 15 buildings.  The first box
   is the building ID (see pictures in img/bldg directory), the second is
   the X coordinate (tile number), and the third is the Y coordinate.
   Note that placing buildings with id 8 or 9 will have unpredictable results.
* The boxes at the bottom denote initial starting angle - 0 is straight up,
   64 is straight right, 128 is down, 192 is straight left.
   The first box is for the Allies and the second is for Axis.
If you do happen to create anything interesting with it, do let me know, and
I might just bundle it with an upcoming version of the game.

A Note on Historical Accuracy
----
Pretty much nothing in this game is historically accurate, especially the events
portrayed in the single player.  I am not particularly interested in the realism
of my game, and do not feel that anachronisms, errors in scale, or poor
attention to detail detracts from the enjoyment.  It isn't meant to be a true
reflection of military history.

Credits
----
Programming, art, design - Greg Kennedy
Additional art           - stock photos lifted from images.google.com
Creative input/feedback  - Steven Silvey
Music                    - 2pac, spoon, h0l, Lone Wolf/Beam,
                           The unconsciousness & Assign
Sound Effects            - located at www.findsounds.com
Beta Testers             - Steven Silvey
                           Abhishek Shrestha
                           ndk5027
                           Thaumaturge
                           Jettoz

Special Thanks to        - Katherine Kennedy <3
                           www.gamedev.net

Contact
----
kennedy.greg@gmail.com
I love hearing your questions, comments, suggestions, criticisms, etc.

License Info
----
Please see license.txt for all software license information.
