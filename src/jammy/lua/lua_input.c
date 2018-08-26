#include <lua.h>
#include <lauxlib.h>

#if defined(JM_LINUX)
#include <X11/keysym.h>
#endif

void jm_luaopen_input(lua_State* L)
{
    lua_getglobal(L, "jam");

	lua_newtable(L);

	lua_pushliteral(L, "key");
    lua_newtable(L);
#define SETKEY(Name, Code) \
	lua_pushliteral(L, #Name); \
	lua_pushinteger(L, (lua_Integer)Code); \
    lua_settable(L, -3)

#if defined(JM_LINUX)
	SETKEY(Return, XK_Return);
	SETKEY(Backspace, XK_BackSpace);

	SETKEY(Escape, XK_Escape);
	SETKEY(Tab, XK_Tab);
	SETKEY(CapsLock, XK_Caps_Lock);

	SETKEY(LeftShift, XK_Shift_L);
	SETKEY(RightShift, XK_Shift_R);
	SETKEY(LeftControl, XK_Control_L);
	SETKEY(RightControl, XK_Control_R);
	SETKEY(LeftAlt, XK_Alt_L);
	SETKEY(RightAlt, XK_Alt_R);

    SETKEY(LeftArrow, XK_Left);
    SETKEY(RightArrow, XK_Right);
    SETKEY(UpArrow, XK_Up);
    SETKEY(DownArrow, XK_Down);

	SETKEY(F1, XK_F1);
	SETKEY(F2, XK_F2);
	SETKEY(F3, XK_F3);
	SETKEY(F4, XK_F4);
	SETKEY(F5, XK_F5);
	SETKEY(F6, XK_F6);
	SETKEY(F7, XK_F7);
	SETKEY(F8, XK_F8);
	SETKEY(F9, XK_F9);
	SETKEY(F10, XK_F10);
	SETKEY(F11, XK_F11);
	SETKEY(F12, XK_F12);

	SETKEY(A, XK_a);
	SETKEY(B, XK_b);
	SETKEY(C, XK_c);
	SETKEY(D, XK_d);
	SETKEY(E, XK_e);
	SETKEY(F, XK_f);
	SETKEY(G, XK_g);
	SETKEY(H, XK_h);
	SETKEY(I, XK_i);
	SETKEY(J, XK_j);
	SETKEY(K, XK_k);
	SETKEY(L, XK_l);
	SETKEY(M, XK_m);
	SETKEY(N, XK_n);
	SETKEY(O, XK_o);
	SETKEY(P, XK_p);
	SETKEY(Q, XK_q);
	SETKEY(R, XK_r);
	SETKEY(S, XK_s);
	SETKEY(T, XK_t);
	SETKEY(U, XK_u);
	SETKEY(V, XK_v);
	SETKEY(W, XK_w);
	SETKEY(X, XK_x);
	SETKEY(Y, XK_y);
	SETKEY(Z, XK_z);
#endif
    

#undef SETKEY
	lua_settable(L, -3);

	lua_pushliteral(L, "input");
	lua_pushvalue(L, -2);
	lua_settable(L, -4);

	lua_pop(L, 1);
}