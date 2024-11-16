This program has been fully included in LunaTranslator from the beginning and is a sub project of the project.

At first, it had no UI at all, only LunaHost.dll&LunaHook.dll for LunaTranslator to use. I just hope to hook some ancient games on Windows XP, so I had to create such a rudimentary UI version.

I don't know why you are obsessed with such a crude program. This rudimentary program lacks many important features, such as embedded translation, which have always been supported only in LunaTranslator.

And now that I have successfully made LuanTranslator directly compatible with Windows XP, I have no reason to keep this compromise product. Maintaining these two similar projects simultaneously is too exhausting for me.

If you insist on using such a rudimentary program, it's very simple. I didn't delete the script that built it. You just need to fork LunaTranslator and use GitHub actions, wait for 3 minutes, and GitHub will automatically build the project for you.

1. Fork [LunaTranslator](https://github.com/HIllya51/LunaTranslator/fork):
   ![](https://p.inari.site/guest/24-11/16/6738474d612e5.png)
2. Open workflow **buildlunahook** in your forked program's Actions
   ![](https://p.inari.site/guest/24-11/16/6738474d77a9e.png)
3. Run this workflow, wait 3 minutes, github will automatically assist you build this program. The built program can download in the release of your own forked project.
   ![](https://p.inari.site/guest/24-11/16/6738474e45378.png)
