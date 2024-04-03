/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

// Based on Bjarne Stroustrup's C++ Style and Technique FAQ
// https://www.stroustrup.com/bs_faq2.html#constraints

template<class T, class B> struct derived_from {
	static void constraints(T* p) {
		B* pb = p;
		(void)pb;
	}
	derived_from() {
		void(*p)(T*) = constraints;
	}
};
