-- See semantics2

import qualified Data.Vector as V
import Data.Monoid

data InstructionType = B | C | K | W | G | P | N | M | NullInstruction
    deriving (Show, Eq)

data InstructionGroup = Black | White
    deriving (Show, Eq)

data Instruction = Instruction
    { insnType :: InstructionType
    , insnAddress :: Maybe Int
    , insnGroup :: InstructionGroup
    }
    deriving (Show, Eq)

-- max size of 4
-- address, data
type ReadHead = (Maybe Int, V.Vector Instruction)

data Machine = Machine
    { readHeads :: V.Vector ReadHead
    , primaryReadHead :: Int
    , programCounter :: Int
    , value :: Maybe Instruction
    , program :: Program
    }
    deriving (Show)

type Program = V.Vector Instruction

numberOfReadHeads = 3
readHeadLength = 4

initialize :: Program -> Machine
initialize program = Machine
    { readHeads = readHeads
    , primaryReadHead = 0
    , programCounter = 0
    , value = Nothing
    , program = program
    }
    where
        readHeads = V.singleton (makeReadHead 0 program) <> emptyReadHeads
        emptyReadHeads = V.generate (numberOfReadHeads - 1) (const emptyReadHead)

prettyPrint :: Machine -> IO ()
prettyPrint machine = do
    putStrLn "Machine"
    putStrLn $ "Read heads:"
    putStrLn $ "- primaryHead: "  ++ show (getPrimaryReadHead machine)
    putStrLn $ "- secondaryHead: "  ++ show (getSecondaryReadHead machine)
    putStrLn $ "- tertiaryHead: "  ++ show (getTertiaryReadHead machine)
    putStrLn $ "-- pointer: " ++ show (primaryReadHead machine)
    putStrLn $ "Program Counter: " ++ show (programCounter machine)
    putStrLn $ "Value: " ++ show (value machine)
    putStrLn $ "Program: " ++ show (program machine)

makeReadHead :: Int -> Program -> ReadHead
makeReadHead addr program = padReadHead (Just addr, V.take readHeadLength . V.drop addr $ program)

padReadHead :: ReadHead -> ReadHead
padReadHead (addr, head) = (addr, padded)
    where
        toPad = readHeadLength - V.length head
        lastGroup = insnGroup . V.last $ head
        padded = head <> emptyProg toPad (toggleGroup lastGroup)

emptyReadHead :: ReadHead
emptyReadHead = (Nothing, emptyProg 4 White)

emptyProg length startingGroup = V.zipWith ($) insns groups
    where
        groups = V.iterateN length toggleGroup startingGroup
        insns = V.generate length (const nullInsn)

toggleGroup Black = White
toggleGroup White = Black

nullInsn = Instruction NullInstruction Nothing
b = Instruction B Nothing
c = Instruction C Nothing
k = Instruction K Nothing
w = Instruction W Nothing
n addr = Instruction N (Just addr)
m addr = Instruction M (Just addr)
g addr = Instruction G (Just addr)
p addr = Instruction P (Just addr)

group g insns = map ($ g) insns

step :: Machine -> Machine
step machine
    | isNest machine = nest machine
    | isReturningFromNest machine = returnFromNest machine

getPrimaryReadHead (Machine{readHeads=readHeads, primaryReadHead=primaryReadHead}) = readHeads V.! (primaryReadHead)
getSecondaryReadHead (Machine{readHeads=readHeads, primaryReadHead=primaryReadHead}) = readHeads V.! ((primaryReadHead + 1) `mod` numberOfReadHeads)
getTertiaryReadHead (Machine{readHeads=readHeads, primaryReadHead=primaryReadHead}) = readHeads V.! ((primaryReadHead + 2) `mod` numberOfReadHeads)

isNest :: Machine -> Bool
isNest machine = insnType firstWord == N
    where
        readHead = getPrimaryReadHead machine
        firstWord = V.head (snd readHead)

nest :: Machine -> Machine
nest machine = machine
    { readHeads = heads V.// [(newPRH, newHead), (pRH, pHead')]
    , primaryReadHead = newPRH
    , programCounter = addr
    }
    where
        newPRH = (pRH - 1) `mod` numberOfReadHeads
        pRH = primaryReadHead machine
        heads = readHeads machine
        pHead = getPrimaryReadHead machine
        sHead = getSecondaryReadHead machine
        tHead = getTertiaryReadHead machine
        insn = V.head (snd pHead)
        addr = case insnAddress insn of
            Just a -> a
            Nothing -> error "No address in nesting instruction"
        newHead = makeReadHead addr (program machine)
        newInsn = insn { insnType = M, insnAddress = fst sHead }
        pHead' = (fst pHead, snd pHead V.// [(0, newInsn)])

getWordGroup :: ReadHead -> Program
getWordGroup (_, words) = V.takeWhile (\ w -> firstGroup == insnGroup w) words
    where
        firstGroup = insnGroup . V.head $ words

isReturningFromNest machine = V.length firstWordGroup == 1
    where
        readHead = getPrimaryReadHead machine
        firstWordGroup = getWordGroup readHead

returnFromNest :: Machine -> Machine
returnFromNest machine = machine
    { readHeads = (readHeads machine) V.// [(pRH, nextReadHead), (alterRH, secondHead')]
    , primaryReadHead = alterRH
    , programCounter = newPC
    }
    where
        pRH = primaryReadHead machine
        alterRH = (pRH + 1) `mod` numberOfReadHeads
        readHead = getPrimaryReadHead machine
        secondHead = getSecondaryReadHead machine
        secondHead' = (fst secondHead, (snd secondHead) V.// [(0, copyInsn')])
        tertRH = getTertiaryReadHead machine
        nextReadHead = case unnestAddr of
            Just a -> makeReadHead a (program machine)
            -- Assume we're at the root unnesting instruction (or a null instruction)
            Nothing -> emptyReadHead
        unnestInsn = V.head . snd $ tertRH
        unnestAddr = insnAddress unnestInsn
        copyInsn = V.head . snd $ readHead
        copyInsn' = copyInsn { insnGroup = (insnGroup . V.head . snd $ secondHead) }
        newPC = case (fst secondHead') of
            Just a -> a
            Nothing -> error "No address in read head"

-- Test programs
nestingProg = V.fromList [n 1 White, n 2 Black, b White]

-- TODO
-- write-back to program (and possibly coherency between read heads?)
-- other reductions
