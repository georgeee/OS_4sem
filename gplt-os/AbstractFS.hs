{-# Language TypeFamilies #-}
module AbstractFS where

import Base
import qualified Data.Sequence as Seq
import qualified Data.Map as M
import Data.Array

newtype Executable = Executable (Process [CmdArg])

data InodeType = FileIT | DirIT | SymLinkIT | ExecutableIT
    deriving Show



data InodeMode = InodeMode { modeRead :: Bool, modeWrite :: Bool, modeExec :: Bool }
type SizeT = Int
type BlockId = Int
type InodeId = Int
type FileName = String

data Inode = FileNode { inodeSize :: SizeT
                      , inodeMode :: InodeMode
                      , inodeBlocks :: Seq.Seq BlockId
                      }
           | SymLinkNode { inodeSize :: SizeT
                         , inodeMode :: InodeMode
                         , inodePath :: FilePath
                         }
           | ExecutableNode { inodeMode :: InodeMode
                            , inodeExecutable :: Executable
                            }
           | DirNode { inodeMode :: InodeMode
                     , inodeEntries :: M.Map FileName InodeId
                     }

inodeType :: Inode -> InodeType
inodeType (FileNode {}) = FileIT
inodeType (SymLinkNode {}) = SymLinkIT
inodeType (ExecutableNode {}) = ExecutableIT
inodeType (DirNode {}) = DirIT

blockSize = 256 --Constant

newtype Block = Block (Array Int Char)

createBlock :: [Char] -> Block
createBlock = Block . listArray (0, blockSize - 1) . (++ ['\0','\0'..])

data FS = FS { fsInodes :: M.Map InodeId Inode
             , fsBlocks :: M.Map BlockId Block
             , fsRootInode :: InodeId
             , fsNextBlockId :: BlockId
             , fsNextInodeId :: InodeId
             }

initFS = let rootInode = DirNode (InodeMode True True True) M.empty
         in FS (M.fromList [(0, rootInode]) M.empty rootInode 0 1
