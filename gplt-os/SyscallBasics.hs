{-# Language TypeFamilies #-}
module SyscallBasics where

import Base
import Data.Sequence ((|>))

data ForkT = ForkT
data ExecT = ExecT
data ExitT = ExitT
data YieldT = YieldT


type instance SysCallArgs ForkT = ()
type instance SysCallArgs ExitT = ()
type instance SysCallArgs YieldT = ()
type instance SysCallArgs ExecT = (FilePath, [CmdArg])

type instance SysCallRes ForkT = Maybe Pid
type instance SysCallRes ExitT = Void
type instance SysCallRes YieldT = ()
type instance SysCallRes ExecT = Either ErrNo ()


instance TagT YieldT where
    tagName = const "yield"
    processSysCall pid mmu seq storage (Context' { cont' = cont })
        = return (seq |> pid, setProcInfo storage pid $ ProcInfo pid mmu cont ())
     
instance TagT ForkT where
    tagName = const "fork"
    processSysCall pid mmu seq storage (Context' { cont' = cont })
       = do let (newPid, storage') = launchProcess storage mmu cont Nothing
            let storage'' = setProcInfo storage' pid $ ProcInfo pid mmu cont $ Just newPid
            return (seq |> pid |> newPid, storage'')

instance TagT ExecT where
    tagName = const "exec"
    processSysCall = undefined
    {-processSysCall pid mmu seq storage (Context' { args' = args, cont' = cont })-}

instance TagT ExitT where
    tagName = const "exit"
    processSysCall _ _ seq storage _ = return (seq, storage)



exit _ _ = mkContext ExitT () undefined
fork next _ _ = mkContext ForkT () next
yield next _ _ = mkContext YieldT () next
exec args next _ _ = mkContext ExecT args next

