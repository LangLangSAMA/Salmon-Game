Name: Lang Cheng
Student Number: 40588155
CSID: q9o0b
CWL: langcl

Collisions:
Used exact collision computation by measuring mesh vertexes.
Debug mode activated by pressing C.
In debug mode, collision will pause game for few seconds, and show the collision point;


Game AI:
Fish will move away from salmon when salmon appears in front of it by checking absolute y-pos difference,
if this value is larger than 150, it means the fish will not collide with the salmon's boundary, otherwise it will.
If the x-pos difference (salmon.x - fish.x) < 0, meaning salmon is on the left side of fish, fish will move up or down
depending on its relative position to the salmon. If x-pos difference > 0, meaning salmon is on the right side of fish,
fish will move along the direction where the salmon is coming, to achieve the shortest path.
The path is also drawn on the screen after enabling debug mode.
The key binds to change frequency is N and M, the current frequency will be printed on terminal after you change it.

Creative Part:
Expected collision will show on the border, it depends on the salmon movement direction.