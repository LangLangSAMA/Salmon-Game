Name: Lang Cheng
Student Number: 40588155
CSID: q9o0b
CWL: langcl

Salmon can be controlled to move by pressing arrow keys or WASD.

Collisions:
Used exact collision computation by measuring mesh vertexes.
Debug mode activated by pressing C.
In debug mode, collision will pause game for few seconds, and show the collision point.

Game AI:
Fish will move away from salmon when salmon appears in front of it by checking absolute y-pos difference,
if this value is larger than 150, it means the fish will not collide with the salmon's boundary, otherwise it will.
If the x-pos difference (salmon.x - fish.x) < 0, meaning salmon is on the left side of fish, fish will move up or down
depending on its relative position to the salmon. If x-pos difference > 0, meaning salmon is on the right side of fish,
the fish will move along the direction where the salmon is coming, to achieve the shortest path and also escape from the salmon.
The path is also drawn on the screen after enabling debug mode.
The key binds to change frequency is N and M, the current frequency will be printed on terminal after you change it.

Creative Part:
Advanced mode can be activated or deactivated by pressing X, which will change the size of the salmon to minimum, and
increase its size by eating fish.
Press V to activate or deactivate the expected collision mode, which will show the collision point on the border and it depends 
on the salmon current movement direction.
After pressing Z, it activates the accurate collision check mode, the new spawned fish collision is now checked by 
using 8 different points around the fish based on its bounding box for better accuracy. The effect can only be seen when
using debug mode, and can observe that the collision check is more accurate, which uses the vertexs around the fish instead
of the centre point of the fish.