{-# Language TypeFamilies, ExistentialQuantification, GADTs #-}
module Base where

import qualified Data.Sequence as Seq
import qualified Data.Map as M

data Void
data VirtualAddress = VirtualAddress
data PhysicalAddress = PhysicalAddress

data MMU = MMU

resolveAddress :: MMU -> Pid -> VirtualAddress -> PhysicalAddress
resolveAddress = undefined

type SecureComputation a b = MMU -> a -> b


secureCall :: SecureComputation a b -> MMU -> a -> b
-- ctl=1 (mode restricted)
secureCall sc mmu a = sc mmu a

data Context' tagT = Context' { tag'  :: tagT
                              , args' :: SysCallArgs tagT
                              , cont' :: Process (SysCallRes tagT)
                              }
data Context = forall tagT . (TagT tagT) => Context { context :: Context' tagT }

packContext' :: TagT tagT => Context' tagT -> Context
packContext' = Context

mkContext :: TagT tagT => tagT -> SysCallArgs tagT -> Process (SysCallRes tagT) -> Context
mkContext tag args cont = packContext' $ Context' tag args cont

type Process r = SecureComputation r Context

type CmdArg = String
type Pid = Int
type ErrNo = Int

type family SysCallArgs tagT :: *
type family SysCallRes tagT :: *

data ProcInfo r = ProcInfo { pid :: Pid, mmu :: MMU, process :: (Process r), prevRes :: r }

data ProcInfoBox 
  where
  MkPIBox :: ProcInfo r -> ProcInfoBox

data ProcInfoStorage = ProcInfoStorage { nextPid :: Pid
                                       , pidMap :: M.Map Pid ProcInfoBox
                                       }

getProcInfoBox :: ProcInfoStorage -> Pid -> Maybe ProcInfoBox
getProcInfoBox storage pid = M.lookup pid (pidMap storage)

setProcInfo :: ProcInfoStorage -> Pid -> ProcInfo r -> ProcInfoStorage
setProcInfo storage pid info = storage{ pidMap = M.insert pid (MkPIBox info) (pidMap storage) }

getPBoxPid (MkPIBox pi) = pid pi
getPBoxMMU (MkPIBox pi) = mmu pi

runToNextContext :: ProcInfo r -> MMU -> Context
runToNextContext ProcInfo{process=process, prevRes=prevRes} mmu = secureCall process mmu prevRes
launchProcess :: ProcInfoStorage -> MMU -> Process r -> r -> (Pid, ProcInfoStorage)
launchProcess storage = let  pid = nextPid storage
                             f = \info -> (pid, setProcInfo storage{ nextPid = pid + 1 } pid info) 
                         in  ((f . ) . ) . ProcInfo pid

logKernelStep :: TagT tagT => Context' tagT -> Pid -> IO ()
logKernelStep (Context' {tag' = tag}) pid = putStrLn $ "Process " ++ (show pid) ++ ": tag " ++ (tagName $ tag)

type PidSeq = Seq.Seq Pid

class TagT tagT where
    tagName :: tagT -> String
    processSysCall :: Pid -> MMU -> PidSeq -> ProcInfoStorage -> Context' tagT -> IO (PidSeq, ProcInfoStorage)

runProcessIteration :: ProcInfo r -> PidSeq -> ProcInfoStorage -> IO (PidSeq, ProcInfoStorage)
runProcessIteration procInfo seq storage = do case runToNextContext procInfo (mmu procInfo) of
                                                Context context' -> do
                                                     logKernelStep context' (pid procInfo)
                                                     processSysCall (pid procInfo) (mmu procInfo) seq storage context'
kernel :: PidSeq -> ProcInfoStorage -> IO ()
kernel seq storage  |Seq.null seq  = return ()
                    |otherwise = do let (pid Seq.:< seq') = Seq.viewl seq
                                    let mProcInfoBox = getProcInfoBox storage pid
                                    case mProcInfoBox of
                                        Just (MkPIBox pc) -> do iterRes <- runProcessIteration pc seq' storage
                                                                uncurry kernel iterRes
                                        Nothing -> putStrLn $ "No such pid in storage"

startKernel :: [Process Void] -> IO()
startKernel = uncurry kernel . foldl step (Seq.empty, ProcInfoStorage 0 M.empty)
    where step (seq, storage) process = let (pid, storage') = launchProcess storage MMU process undefined
                                        in  (seq Seq.|> pid, storage')

