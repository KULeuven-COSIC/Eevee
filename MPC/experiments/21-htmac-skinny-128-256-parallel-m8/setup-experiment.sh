cd MP-SPDZ
TARGET=$(cat target)
mkdir -p Programs/Schedules
mv ../${TARGET}.sch Programs/Schedules/
mkdir -p Programs/Bytecode
mv ../${TARGET}-*.bc Programs/Bytecode/