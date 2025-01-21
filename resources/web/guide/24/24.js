let strParentNodeOther = "Other";
let strParentNodeCreality = "Creality";
let strCurrentVender = ""
let machinesCollection = {};


let userPresetCurList = [] // 当前用户预设列表数据
let curSelectTree = '' // 当前选择的左侧数数据
let crealityPrinterClass = [] // creality分类
let allBrandPrinterObj = {} // 所有打印机数据分类
let searchPrinterList = [] // 搜索的打印机
let isChangeColor = false // 默认不改变主题色
let isFirstLoad = false // 是否第一次打开弹框

function OnInit()
{
	//let strInput=JSON.stringify(cData);
	// HandleStudio(strInput);

    TranslatePage();
	
	RequestProfile();
}

function RequestProfile()
{
	var tSend={};
	tSend['sequence_id']=Math.round(new Date() / 1000);
	// tSend['command']="request_userguide_profile";
	tSend['command']="request_userguide_profile_printer";

	SendWXMessage( JSON.stringify(tSend) );
}

function HandleStudio( pVal )
{
	let strCmd=pVal['command'];

	if(strCmd=='response_userguide_profile')
	{
        isChangeColor = pVal.color_mode === '0'
        pVal.color_mode === '0' ? changeTheme() : ''
        // MachineList(pVal['MachineJson']);
		// HandleModelList(pVal['response']);
        // 初始化左侧数数据
        initTreeNavList (pVal.response.model, pVal.MachineJson.machine)
        handelUserPreset(pVal.response.model, pVal.user_preset)
        // 当前用户（0：普通，隐藏切换按钮，1：专家，显示切换按钮）
        pVal.role_type === '0' ? $('#toggle-user-type').hide() : $('#toggle-user-type').show()
	}
}

/**
 * 取消弹框
 */
function CancelSelect() {
    // 如果没选一个打印机，阻止提交数据，并弹出对话框
    // let isSelectPrinter = []
    // for (let key in allBrandPrinterObj) {
    //     // 每个key对应的数组是否选择了
    //     let isSelect = allBrandPrinterObj[key].every(item => item.nozzleSelected.length === 0)
    //     isSelectPrinter.push(isSelect)
    // }
    if (isFirstLoad) {
        $(".dialog").show()
        return
    }
    var tSend={};
    tSend['sequence_id']=Math.round(new Date() / 1000);
    tSend['command']="user_guide_cancel";
    tSend['data']={};
    SendWXMessage(JSON.stringify(tSend));
}

/**
 * 提交数据
 */
function submit () {
    // 如果没选一个打印机，阻止提交数据，并弹出对话框
    let isSelectPrinter = []
    for (let key in allBrandPrinterObj) {
        // 每个key对应的数组是否选择了
        let isSelect = allBrandPrinterObj[key].every(item => item.nozzleSelected.length === 0)
        isSelectPrinter.push(isSelect)
    }
    if (isSelectPrinter.every(item => item === true)) {
        $(".dialog").show()
        return
    }
    // 用户预设字段
    const unSelectUserPreset = userPresetCurList.filter(item => item.isChecked === false)
    // 选择的打印机
    let allPrinterList = []
    for (let key in allBrandPrinterObj) {
        allPrinterList.push(...allBrandPrinterObj[key])
    }
    const selectedPrinterList = allPrinterList.reduce((acc, cur) => {
        if (cur.nozzleSelected.length) {
            acc[cur.model] = {
                model: cur.model,
                nozzle_diameter: cur.nozzleSelected.join(';'),
                vendor: cur.vendor
            }
        }
        return acc
    }, {})
    // 调用存储命令
    const data = {
        sequence_id: Math.round(new Date() / 1000),
        command: 'save_userguide_models',
        data: selectedPrinterList,
        user_data: unSelectUserPreset.length ? unSelectUserPreset.map(item => {
            return {
                name: item.printer_name
            }
        }) : []
    }
    SendWXMessage(JSON.stringify(data))
    // 调用结束命令
    const finish = {
        sequence_id: Math.round(new Date() / 1000),
        command: 'user_guide_finish',
        data: {
            action: 'finish'
        },
        user_data: []
    }
    SendWXMessage(JSON.stringify(finish))
}
/**
 * 确认未选择打印机
 */
function confirmSelectDialog () {
    $(".dialog").hide()
    return
}

/**
 * 生成左侧导航树
 * @param {@Array} modelList 
 * @returns 
 */

function initTreeNavList (modelList, machineJson) {
    if (!modelList || !modelList.length) return
    // 过滤掉系列为空的打印机型号数组
    const machineList = machineJson && machineJson.reduce((acc, cur) => {
        if (cur.printers.length) {
            acc.push(cur.name)
        }
        return acc
    }, [])
    // 赋值给全局crealityPrinterClass对象
    crealityPrinterClass = machineJson && machineJson.filter(item => item.printers.length !== 0)
    // 归类：1.Creality产品(从另外一个json文件做筛查)  2.其他
    let treeNavObj = { "Creality": [...machineList], "Other": [] }
    // 按照类名数组合并
    const treeDataList =  modelList.reduce((acc, cur) => {
        const key = cur.vendor
        const curGroup = acc[key] ?? []

        // 字符串转数组， 并且把0.4排序到第一
        const nozzleSelected = cur.nozzle_selected.split(';').filter(item => item.length !== 0).sort((a, b) => a > b ? 1 : -1)
        const nozzleDiameter = cur.nozzle_diameter.split(';').sort((a, b) => a > b ? 1 : -1)
        const indexSelected = nozzleSelected.indexOf('0.4')
        const indexDiameter = nozzleDiameter.indexOf('0.4')
        if (indexSelected !== -1) {
            const moveEle = nozzleSelected.splice(indexSelected, 1)
            nozzleSelected.unshift(...moveEle)
        }
        if (indexDiameter !== -1) {
            const moveEle = nozzleDiameter.splice(indexDiameter, 1)
            nozzleDiameter.unshift(...moveEle)
        }
        cur = {
            ...cur,
            nozzleDiameter,
            nozzleSelected,
            isShowCheckBox: false
        }
        return { ...acc, [key]: [...curGroup, cur]}
    }, {})
    treeNavObj.Other = Object.keys(treeDataList).filter(item => item !== 'Creality')
    treeNavObj.Other = treeNavObj.Other.sort((a, b) => a > b ? 1 : -1)
    // 赋值给全局allBrandPrinterObj
    allBrandPrinterObj = treeDataList
    // 是否是第一次加载dialog
    let isSelectPrinter = []
    for (let key in allBrandPrinterObj) {
        // 每个key对应的数组是否选择了
        let isSelect = allBrandPrinterObj[key].every(item => item.nozzleSelected.length === 0)
        isSelectPrinter.push(isSelect)
    }
    isFirstLoad = isSelectPrinter.every(item => item === true)
    // 渲染html  rendTreeNavList()
    rendTreeNavList(treeNavObj)
}

/**
 * 渲染左侧树
 * @param {Array} treeData 
 */
function rendTreeNavList (treeData) {
    // 获取数列表数据
    $.each(treeData, function (key, val) {
        const html = `
                <div class="tree-wrapper">
                    <div class="tree-title" >
                        <div class="tree-title-wrapper" onclick="toggleTreeNav(${key === 'Creality' ? 0 : 1})">
                            <svg
                                id="${key}Svg"
                                xmlns="http://www.w3.org/2000/svg"
                                xmlns:xlink="http://www.w3.org/1999/xlink"
                                width="12px" height="12px">
                                <path fill-rule="evenodd"  fill="none"
                                d="M-0.000,-0.000 L12.000,-0.000 L12.000,12.000 L-0.000,12.000 L-0.000,-0.000 Z"/>
                                <path fill-rule="evenodd"  fill="${isChangeColor ? '#333' : 'rgb(255, 255, 255)'} "
                                d="M8.597,7.746 C8.755,7.913 8.948,7.997 9.172,7.997 C9.395,7.997 9.587,7.913 9.745,7.746 C9.910,7.579 9.993,7.379 9.993,7.146 C9.993,6.913 9.910,6.713 9.745,6.546 L6.567,3.214 C6.498,3.130 6.413,3.071 6.312,3.039 C6.210,3.006 6.102,2.993 5.988,2.998 C5.874,3.004 5.763,3.030 5.651,3.074 C5.542,3.119 5.447,3.183 5.368,3.267 L2.238,6.546 C2.075,6.713 1.992,6.913 1.992,7.146 C1.992,7.379 2.075,7.579 2.238,7.746 C2.318,7.830 2.408,7.893 2.506,7.935 C2.606,7.976 2.708,7.997 2.812,7.997 C2.918,7.997 3.022,7.976 3.124,7.935 C3.224,7.893 3.316,7.830 3.396,7.746 L5.998,5.041 L8.597,7.746 Z"/>
                            </svg>
                            <span style="color: ${isChangeColor ? '#333' : '#fff'};font-weight: bold">${key}</span>
                        </div>
                    </div>
                    <div class="tree-node-list" id="${key}">
                    </div>
                </div>
             `
        $("#tree-container").append(html)
    })
    // 直接写有bug，一步步append
    const crealityList = treeData.Creality
    $.each(crealityList, function (index, val) {
        const html = `<div class="tree-node-item" style="color: ${isChangeColor ? '#333' : '#fff'}" onclick="getCurrentTree(this)" id="${val}">${val}</div>`
        $("#Creality").append(html)
    })
    const otherList = treeData.Other
    $.each(otherList, function (index, val) {
        const html = `<div class="tree-node-item" style="color: ${isChangeColor ? '#333' : '#fff'}" onclick="getCurrentTree(this)" id="${val}">${val}</div>`
        $("#Other").append(html)
    })
    getCurrentTree()
}
/**
 * 改变主题色
 */

function changeTheme () {
    // 按钮背景色切换
    $('#sysBtn').toggleClass('toggle-button-white toggle-button')
    $('#userBtn').toggleClass('toggle-button-white toggle-button')

    document.body.style.backgroundColor = "#fff";
    document.body.classList.add("white-theme");

    // 头部背景色切换
    $('#toggle-user-type').toggleClass('toggle-user-type toggle-user-type-white')

    // 搜索区域颜色切换
    // 搜索框颜色切换
    // 左侧树
    
    // 左侧树数据字体颜色
    $('.tree-node-item').toggleClass('tree-node-item tree-node-item-white')
    // 底部按钮颜色
    // checkbox框的颜色
    $('input[type=checkbox]').append(`<style>input[type="checkbox"]::after {background: #fff;}</style>`)
    // 底部按钮颜色
    $('.acceptArea').css({ background: '#fff'})
    // 搜索框颜色
    $('#searchPrinterAll').css({ 'color': '#333' })
    $('.printer-search-icon').find('path').attr('fill', '#333')
    // 搜索为空时
    $('.search-printer-empty').css({ background: '#fff', color: '#333' })

    /** 用户模块 */
    // wrapper
    $('#userContainer').css({  background: '#fff', color: '#333' })
    $('.select-container').css({  background: '#fff', color: '#333' })
    $('.select-container label').css({ color: '#333' })
    $('.select-container input').css({ background: '#fff' })
    $('.select-container select').css({ background: '#fff' })
    $('.search-icon').find('path').attr('fill', '#333')


    // 打印机卡片区域
    $('#cardContainer').css({  background: '#fff', color: '#333' })
}

function renderTreeCard (cardList) {
    $(".card-wrapper").empty()
    let html = ''
    // 置灰按钮，未选择打印机喷嘴时才渲染
    const unSelectedBtn = `<button type="button" class="card-list-item-button un-selected-btn">0.4mm</button>`
    if (cardList && cardList.length) {
        $.each(cardList, function (index, item) {
            // checkbox循环渲染
            let checkboxHtml = ''
            $.each(item.nozzleDiameter, function (index, val) {
                checkboxHtml += `
                   <div>
                       <input type="checkbox" ${item.nozzleSelected.includes(val) ? `checked` : ``} value=\"${item.model}\" onchange="selectTreeCardNozzle(this, event)">
                       <label for="" name="checkbox" id="${index + item.model}">${val}mm</label>
                   </div>
               `
           })
            // button循环渲染
           let btnHtml = ''
           $.each(item.nozzleSelected, function (idx, val) {
                btnHtml += `
						<button type="button" class="card-list-item-button" style="background: ${isChangeColor ? '#fff' : '#74747B' }; border: ${isChangeColor ? '1px solid #D6D6DC' : '1px solid #6E6E72'}">
                            <span style="color: ${isChangeColor ? '#333' : '#fff' }">${val}mm</span>
							<svg
                                data-model=\"${item.model}\"
                                onclick="removePrinterNozzle(this, event)"
								class="card-list-item-remove"
								xmlns="http://www.w3.org/2000/svg"
								xmlns:xlink="http://www.w3.org/1999/xlink"
								width="23px" height="23px">
								<path fill-rule="evenodd" stroke="${isChangeColor ? '#eee' : 'rgb(93, 93, 97)'}" stroke-width="2px" stroke-linecap="butt" stroke-linejoin="miter" fill="rgb(255, 255, 255)"
								d="M10.999,1.000 C16.523,1.000 21.000,5.477 21.000,11.000 C21.000,16.523 16.523,21.000 10.999,21.000 C5.477,21.000 1.000,16.523 1.000,11.000 C1.000,5.477 5.477,1.000 10.999,1.000 Z"/>
								<path fill-rule="evenodd"  fill="rgb(75, 75, 77)"
								d="M11.864,10.999 L13.794,9.062 C14.036,8.819 14.036,8.425 13.794,8.182 C13.552,7.939 13.159,7.939 12.918,8.182 L10.987,10.119 L9.057,8.182 C8.816,7.939 8.423,7.939 8.181,8.182 C7.939,8.425 7.939,8.819 8.181,9.062 L10.110,10.999 L8.181,12.937 C7.939,13.180 7.939,13.574 8.181,13.817 C8.297,13.934 8.455,14.000 8.619,13.999 C8.784,14.000 8.941,13.934 9.057,13.817 L10.987,11.880 L12.918,13.817 C13.033,13.934 13.191,14.000 13.356,13.999 C13.520,14.000 13.678,13.934 13.794,13.817 C14.036,13.574 14.036,13.180 13.794,12.937 L11.864,10.999 Z"/>
							</svg>
						</button>
                `
           })

        //    const isNeedChangePosition = (index + 1) % 3 === 0
           // 向左移动 80px 100px 150px
           const changePosition = item.nozzleSelected.length >= 3 ? ((Math.floor(item.nozzleSelected.length / 3) + 1) * 32 + 'px') : '30px'

           // 判断是否只有一个喷嘴且为0.4mm，只有一个不显示悬浮弹窗，多个就显示
           const isShowHoverBtn = (item.nozzleDiameter.length === 1 && item.nozzleDiameter[0] === '0.4') ? `` : 
                        `<button data-index=${index} style="background: ${isChangeColor ? '#fff' : '#5D5D61'}; border: ${isChangeColor ? '1px solid #D6D6DC' : '1px solid #6E6E72'}" data-value=${item.nozzleSelected} data-model=\"${item.model}\" type="button" class="card-add-printer-button" onmouseover="isShowCheckboxArea(this)" onmouseout="hideCheckBox(this, event)">
							<svg
								xmlns="http://www.w3.org/2000/svg"
								xmlns:xlink="http://www.w3.org/1999/xlink"
								width="20px" height="20px">
								<path fill-rule="evenodd"  fill="none"
									d="M-0.000,-0.000 L20.000,-0.000 L20.000,20.000 L-0.000,20.000 L-0.000,-0.000 Z"/>
								<path fill-rule="evenodd"  fill="${isChangeColor ? '#333' : 'rgb(255, 255, 255)'}"
									d="M14.799,8.322 L11.199,8.322 L11.199,11.983 C11.199,12.656 10.662,13.203 10.000,13.203 C9.337,13.203 8.800,12.656 8.800,11.983 L8.800,8.322 L5.199,8.322 C4.537,8.322 3.999,7.775 3.999,7.101 C3.999,6.427 4.537,5.881 5.199,5.881 L8.800,5.881 L8.800,2.220 C8.800,1.784 9.028,1.381 9.400,1.163 C9.771,0.945 10.228,0.945 10.599,1.163 C10.971,1.381 11.199,1.784 11.199,2.220 L11.199,5.881 L14.799,5.881 C15.462,5.881 15.999,6.427 15.999,7.101 C15.999,7.775 15.462,8.322 14.799,8.322 ZM5.199,16.559 L14.799,16.559 C15.462,16.559 15.999,17.105 15.999,17.779 C15.999,18.453 15.462,19.000 14.799,19.000 L5.199,19.000 C4.537,19.000 3.999,18.453 3.999,17.779 C3.999,17.105 4.537,16.559 5.199,16.559 Z"/>
						   </svg>
						   <!-- 弹框里面的checkbox -->
						   <div class="card-list-item-checked-printer-wrapper" data-model=\"${item.model}\"  onmouseout="closeCheckBox(this)" style="display: ${item.isShowCheckBox ? 'block': 'none'}; top: ${ changePosition }; background: ${ isChangeColor ? '#F6F6F9' : '#4B4B4D' }; border-color: ${isChangeColor ? '#D6D6DC' : '#6E6E72'}">
								<div class="card-list-item-checked-printer">
                           `
                             +   checkboxHtml +
                                
							`	</div>
						   </div>
						</button>
                        `
            html = `
                <div class="card-list-item" style="background: ${isChangeColor ? '#f6f6f9' : '#5D5D61'}; border-color: ${isChangeColor ? '#D6D6DC' : '#6E6E72'}" data-id=\"${item.model}\">
					<!-- 标题 -->
					<div class="card-item-title" onclick="toggleChecked(this, event)">
						<span style="color: ${isChangeColor ? '#333' : '#fff' }">${item.model}</span>
						<div>
							<label for="checkAll" name="checkbox" id="checkbox"></label>
							<input type="checkbox" data-model=\"${item.model}\" id=\"${item.model}\" class="printer-checkbox" ${item.nozzleSelected.length ? `checked` : ``} onclick="selectPrinterCard(this, event)">
						</div>
					</div>
					<!-- 尺寸 -->
					<div class="card-list-size" style="color: ${isChangeColor ? '#999' : '#ADADAD' }">${item.area ? item.area + ' mm' : ''}</div>
					<!-- 图片 -->
					<div class="card-list-img" onclick="toggleChecked(this, event)">
						<img src=\"${item.cover}\" alt="">
					</div>
					<!-- 孔径 -->
					<div class="card-list-aperture trans" tid="t117" style="color: ${isChangeColor ? '#999' : '#ADADAD' }">喷嘴孔径</div>
					<!-- 添加喷嘴 -->
					 <div class="card-list-add-nozzle"> ` + btnHtml + `${item.nozzleSelected.length ===0 ? unSelectedBtn : ''}` 
                        +
                        isShowHoverBtn
                        +
					 `</div>
				</div>
            `
            $(".card-wrapper").append(html)
        }) 
    }
    // 翻译
    TranslatePage(); 
}

/**
 * 生成左侧树
 * @param {HTMLElement} val 
 */
function getCurrentTree (val) {
    // 移除其他元素的样式
    $("#tree-container").find('.tree-node-item').removeClass('is-selected')
    if (val) {
        $(val).addClass("is-selected")
    }

    const currentTree =  val ? $(val).text() : ''
    curSelectTree = currentTree
    // 如果未选择数，默认第一个为选择的树元素
    if (curSelectTree === '') {
        $("#Creality").children().first().addClass('is-selected')
    }
    // 获取点击分类名、当前点击文本内容
    const treeType = $(val).parent().attr('id') || ''
    // 注：初始化页面打开，默认选择Creality第一个元素渲染
    if (treeType === 'Creality' || !curSelectTree.length) {
        // 所有creality打印机数据
        const allCrealityPrinter = allBrandPrinterObj.Creality
        // 筛选当前点击系列
        const curPrinterClass = crealityPrinterClass.reduce((acc, cur, index) => {
            if (cur.name === currentTree) {
                acc = cur.printers
            } else if (!curSelectTree.length && index === 0) { // 当curSelectTree为空是，默认选择第一个
                acc = cur.printers
            }
            return acc
        }, '')
        // 筛选数据
        const curPrinterList = allCrealityPrinter.reduce((acc, item) => {
            if (curPrinterClass.includes(item.model)) acc.push(item)
            return acc
        }, [])
        // 渲染卡片  
        renderTreeCard(curPrinterList)
    } else {
        const curPrinterList = allBrandPrinterObj[currentTree]
        renderTreeCard(curPrinterList)
    }
    isSelectAllPrinter()
}

 /**
  * 点击移除按钮，删除当前已选喷嘴
  * @param {HTML} val 
  */
 function removePrinterNozzle (val, event) {
    event.stopPropagation()
    // 获取当前型号、喷嘴、所属分类
    const currentNozzle = $(val).parent().find('span').text().split('mm')[0]
    const currentModel = $(val).data('model')
    // 修改数据。移除currentNozzle
    for (let key in allBrandPrinterObj) {
        allBrandPrinterObj[key].forEach(item => {
            if (item.model === currentModel) {
                item.nozzleSelected = item.nozzleSelected.filter(item => item !== currentNozzle)
            }
        })
    }
    // 重新渲染卡片数据
    let curPrinterKey = ''
    for (let key in allBrandPrinterObj) {
        allBrandPrinterObj[key].forEach(item => {
            if (item.model === currentModel) {
                curPrinterKey = key
            }
        })
    }
    // creality属于特殊情况，再进行一次判断
    if (curPrinterKey === 'Creality') {
        // 属于哪个系列        
        const crealityKey = crealityPrinterClass.reduce((acc, cur) => {
            if (cur.printers.includes(currentModel)) {
                acc = cur.name
            }
            return acc
        }, '')
        // 所有creality打印机数据
        const allCrealityPrinter = allBrandPrinterObj.Creality
        // 筛选当前点击系列
        const curPrinterClass = crealityPrinterClass.reduce((acc, cur) => {
            if (cur.name === crealityKey) {
                acc = cur.printers
            }
            return acc
        }, '')
        // 筛选数据
        let curPrinterList = allCrealityPrinter.reduce((acc, item) => {
            if (curPrinterClass.includes(item.model)) acc.push(item)
            return acc
        }, [])
        // 如果有searchPrinterList 设置过滤
        if (searchPrinterList.length) {
            renderTreeCard(searchPrinterList)
        } else {
            renderTreeCard(curPrinterList)
        }
    } else {
        if (searchPrinterList.length) {
            renderTreeCard(searchPrinterList)
        } else {
            const curPrinterList = allBrandPrinterObj[curPrinterKey]
            renderTreeCard(curPrinterList)
        }
    }
    isSelectAllPrinter()
}

/**
 * 切换checkbox事件
 * @param {HTML元素} val 
 */
function selectTreeCardNozzle (val, event) {
    event.stopPropagation()
    const isChecked = $(val).prop('checked')
    // 获取当前型号、喷嘴、所属分类
    const currentModel = $(val).val()
    const currentNozzle = $(val).next().text().split('mm')[0]
    // 修改数据
    for (let key in allBrandPrinterObj) {
        allBrandPrinterObj[key].forEach(item => {
            if (item.model === currentModel) {
                // 修改显示状态
                item.isShowCheckBox = true
                if (isChecked) {
                    item.nozzleSelected.push(currentNozzle)
                } else {
                    item.nozzleSelected = item.nozzleSelected.filter(item => item !== currentNozzle)
                }
            }
        })
    }
    // 重新渲染卡片数据
    let curPrinterKey = ''
    for (let key in allBrandPrinterObj) {
        allBrandPrinterObj[key].forEach(item => {
            if (item.model === currentModel) {
                curPrinterKey = key
            }
        })
    }
    // creality属于特殊情况，再进行一次判断
    if (curPrinterKey === 'Creality') {
        // 属于哪个系列        
        const crealityKey = crealityPrinterClass.reduce((acc, cur) => {
            if (cur.printers.includes(currentModel)) {
                acc = cur.name
            }
            return acc
        }, '')
        // 所有creality打印机数据
        const allCrealityPrinter = allBrandPrinterObj.Creality
        // 筛选当前点击系列
        const curPrinterClass = crealityPrinterClass.reduce((acc, cur) => {
            if (cur.name === crealityKey) {
                acc = cur.printers
            }
            return acc
        }, '')
        // 筛选数据
        let curPrinterList = allCrealityPrinter.reduce((acc, item) => {
            if (curPrinterClass.includes(item.model)) acc.push(item)
            return acc
        }, [])
        // 如果有searchPrinterList 设置过滤
        if (searchPrinterList.length) {
            renderTreeCard(searchPrinterList)
        } else {
            renderTreeCard(curPrinterList)
        }
    } else {
        // 如果有searchPrinterList 设置过滤
        if (searchPrinterList.length) {
            renderTreeCard(searchPrinterList)
        } else {
            const curPrinterList = allBrandPrinterObj[curPrinterKey]
            renderTreeCard(curPrinterList)
        }
    }
    isSelectAllPrinter()
}

/**
 * 设置当前全选状态
 * @param {string} val 
 */
function selectAllPrinter (val) {
    const isChecked = $(val).prop('checked')
    // 获取当前分类下的数据
    const curTree = curSelectTree ?  curSelectTree : crealityPrinterClass[0].name
    // 1.creality下面的分类
    const isCrealityClass = crealityPrinterClass.some(item => item.name === curTree) || curTree === ''
    if (isCrealityClass) {
        const allCrealityPrinter = allBrandPrinterObj.Creality
        const curPrinterClass = crealityPrinterClass.reduce((acc, cur) => {
            if (cur.name === curTree) {
                acc = cur.printers
            }
            return acc
        }, '')
        for (let key in allBrandPrinterObj) {
            if (key === 'Creality') {
                allBrandPrinterObj[key].forEach(item => {
                    if (curPrinterClass.includes(item.model)) {
                        item.nozzleSelected = isChecked ? item.nozzleDiameter : []
                    }
                })
            }
        }
        // 筛选数据
        const curPrinterList = allCrealityPrinter.reduce((acc, item) => {
            if (curPrinterClass.includes(item.model)) acc.push(item)
            return acc
        }, [])
        renderTreeCard(curPrinterList)
    } else {
        // 2.其他分类
        for (let key in allBrandPrinterObj) {
            if (key === curTree) {
                allBrandPrinterObj[key].forEach(item => {
                    item.nozzleSelected = isChecked ? item.nozzleDiameter : []
                })
            }
        }
        renderTreeCard(allBrandPrinterObj[curTree])
    }
}

function selectPrinterCard (val, event) {
    event.stopPropagation()
    const isChecked = $(val).prop('checked')
    const currentModel = $(val).data('model')
    // 查找当前元素的值，修改数据
    for (let key in allBrandPrinterObj) {
        allBrandPrinterObj[key].forEach(item => {
            if (item.model === currentModel) {
                // 如果true,勾选
                if (isChecked) {
                    if (item.nozzleSelected.includes('0.4')) return
                    item.nozzleSelected.unshift('0.4')
                    // item.nozzleSelected = item.nozzleSelected.filter(item => item)
                } else {
                    item.nozzleSelected = []
                }
            }
        })
    }
    // 获取当前分类下的数据
    const curTree = curSelectTree ?  curSelectTree : crealityPrinterClass[0].name
    // 1.creality下面的分类
    const isCrealityClass = crealityPrinterClass.some(item => item.name === curTree) || curTree === ''
    if (isCrealityClass) {
        const allCrealityPrinter = allBrandPrinterObj.Creality
        const curPrinterClass = crealityPrinterClass.reduce((acc, cur) => {
            if (cur.name === curTree) {
                acc = cur.printers
            }
            return acc
        }, '')
        // 筛选数据
        let curPrinterList = allCrealityPrinter.reduce((acc, item) => {
            if (curPrinterClass.includes(item.model)) acc.push(item)
            return acc
        }, [])
        // 如果有searchPrinterList 设置过滤掉当前搜索出来的printerList
        if (searchPrinterList.length) {
            // curPrinterList = curPrinterList.filter(item => item.model === currentModel)
            renderTreeCard(searchPrinterList)
        } else {
            renderTreeCard(curPrinterList)
        }
    } else {
        // 2.其他分类
        if (searchPrinterList.length) {
            renderTreeCard(searchPrinterList)
        } else {
            renderTreeCard(allBrandPrinterObj[curTree])
        }
    }
}

/**
 * 判断是否全选
 */
function isSelectAllPrinter () {
    let isSelectedAll = false
    const curTree = curSelectTree ? curSelectTree : crealityPrinterClass[0].name
    // 获取当前分类下面的数据
    // 1.creality情况
    const isCrealityClass = crealityPrinterClass.some(item => item.name === curTree) || curTree === ''
    if (isCrealityClass) {
        const allCrealityPrinter = allBrandPrinterObj.Creality
        const curPrinterClass = crealityPrinterClass.reduce((acc, cur) => {
            if (cur.name === curTree) {
                acc = cur.printers
            }
            return acc
        }, '')
        // 筛选数据
        const curPrinterList = allCrealityPrinter.reduce((acc, item) => {
            if (curPrinterClass.includes(item.model)) acc.push(item)
            return acc
        }, [])
        isSelectedAll = curPrinterList.every(item => item.nozzleSelected.length === item.nozzleDiameter.length)
    } else {
        // 2.其他
        isSelectedAll = allBrandPrinterObj[curTree].every(item => item.nozzleSelected.length === item.nozzleDiameter.length)
    }
    isSelectedAll ? $("#checkAllPrinter").prop('checked', true) : $("#checkAllPrinter").prop('checked', false)
}

function toggleChecked (val, event) {
    event.stopPropagation()
    $(val).parent().find('input[type="checkbox"].printer-checkbox').click()
    isSelectAllPrinter()
}

/**
 * 鼠标悬浮上去后，显示弹框
 * @param {HTML} val 
 */
function isShowCheckboxArea (val) {
    $(val).find('.card-list-item-checked-printer-wrapper').show()
    $(val).addClass('hover-bth-selected')
    $(val).find('svg path:nth-of-type(2)').attr('fill', isChangeColor ? '#17CC5F' : '#fff')
}

function hideCheckBox (val, event) {
    event.stopPropagation()
    const element = $(val).find('.card-list-item-checked-printer-wrapper')
    closeCheckBox(element)
    $(val).removeClass('hover-bth-selected')
    $(val).find('svg path:nth-of-type(2)').attr('fill', isChangeColor ? '#333' : 'rgb(255, 255, 255)')
}

function closeCheckBox (val) {
    const currentModel = $(val).data('model')
    for (let key in allBrandPrinterObj) {
        allBrandPrinterObj[key].forEach(item => {
            if (item.model === currentModel) {
                item.isShowCheckBox = false
            }
        })
    }
    $(val).hide()
}

/**
 * 清空搜索框
 */
function clearSearch () {
    $("#searchPrinterAll").val('')
    $(".search-printer-empty").css({'display': 'none'})
    searchPrinterModel($("#searchPrinterAll"))
}
/**
 * 搜索框搜索
 * @param {string} val 
 */
function searchPrinterModel (val) {
    const searchVal = $(val).val().replace(/\s+/g, "").toLowerCase() // 去掉空格
    if (searchVal.length) {
        $("#tree-container").hide()
        $("#search-result").show()
        $(".printer-search-remove").show()
        $(".selectAllBtn").hide()
        let timer = null
        if (timer) clearTimeout(timer)
        timer = setTimeout(() => {
            const curPrinterList = []
            for (let key in allBrandPrinterObj) {
                allBrandPrinterObj[key].forEach(item => {
                    if (item.model.replace(/\s+/g, "").toLowerCase().includes(searchVal)) {
                        curPrinterList.push(item)
                    }
                })
            }
            searchPrinterList = curPrinterList
            !searchPrinterList.length ? $(".search-printer-empty").css({'display': 'flex'}) : $(".search-printer-empty").css({'display': 'none'})
            renderTreeCard(curPrinterList)
        }, 300)
    } else {
        $("#tree-container").show()
        $(".selectAllBtn").show()
        $("#search-result").hide()
        $(".printer-search-remove").hide()
        setTimeout(() => {
            getCurrentTree()
            // 清空searchPrinterList
            searchPrinterList = []
        }, 300)
    }
}
/**
 * 点击折叠树
 * @param {number} id 
 */
function toggleTreeNav (id) {
    const idSelector = id === 0 ? "#Creality" : "#Other"
    $(idSelector).toggle()
    const isCrealityVisible = $("#Creality").is(":visible")
    const isOtherVisible = $("#Other").is(":visible")
    const svgSelector = id === 0 ? "#CrealitySvg" : "#OtherSvg"
    // 两个判断写一起就会有bug
    if (id === 0) {
        !isCrealityVisible ? $(svgSelector).css("transform", "rotate(180deg)") : $(svgSelector).css("transform", "rotate(0deg)")
    } else {
        !isOtherVisible ? $(svgSelector).css("transform", "rotate(180deg)") : $(svgSelector).css("transform", "rotate(0deg)")
    }
 }

/**
 * 点击切换用户、系统
 */
function toggleSys () {
    $('.content-wrapper').css({ height: 'calc(100vh - 73px)' })
    $('#userContainer').hide()
    $('#main-container').show()
    $('.wrap').show()
    $('#userBtn').removeClass('toggle-button-isChecked')
    $('#sysBtn').addClass('toggle-button-isChecked')
}

function toggleUser () {
    $('.content-wrapper').css({ height: 'calc(100vh - 40px)' })
    $('#userContainer').show()
    $('#main-container').hide()
    $('.wrap').hide()
    $('#sysBtn').removeClass('toggle-button-isChecked')
    $('#userBtn').addClass('toggle-button-isChecked')
}

/**
 * 取消关闭弹框
 */
function cancel() {
	var tSend={}
	tSend['sequence_id']=Math.round(new Date() / 1000)
	tSend['command']="user_guide_cancel"
	tSend['data']={}
	SendWXMessage( JSON.stringify(tSend) );		
}

/**
 * modelList @array, userPresetList @array
 * 由用户预设拿到品牌数据和机型数据
 */
function handelUserPreset (modelList, userPresetList) {
    // 先拿到用户预设数据，再从model里面拿到机型数据，从里面挑选出机型对应的品牌（vendor）
    let userPresetArr = []
    if (userPresetList.length) { // 生成机型数据
        userPresetArr = userPresetList.map(preset => {
            const vendorArr = modelList.filter(item => item.model === preset.printer_model)
            const vendor = vendorArr.length ? vendorArr[0].vendor : ''
            const imgSrc = vendorArr.length ? vendorArr[0].cover : ''
            return {
                ...preset,
                vendor,
                imgSrc,
                isChecked: preset.user_presets
            }
        })
    }
    // 赋值给当前用户列表数据
    userPresetCurList = userPresetArr
    // 获取品牌下拉框
    userPresetArr.length && getBrandSelectList(userPresetArr)
    // 获取机型下拉框
    userPresetArr.length && getMachineSelectList(userPresetArr)
    // 渲染卡片
    userPresetArr.length && renderCard(userPresetArr)
    // 设置全选
    userPresetArr.length && isSelectAll()
    
}

function renderCard (userPresetArr) {
    if (userPresetArr.length) {
        $.each(userPresetArr, function (index, item) {
            const cardHTML =  `
                <div class="card-area" style="background: ${isChangeColor ? '#fff' : '#5D5D61' }; border: ${isChangeColor ? '1px solid #D6D6DC' : '1px solid #6E6E72'}">
                    <div class="card-header">
                        <span>${item.printer_model}</span>
                        <div>
                            <label for="checkbox"></label>
                            <input type="checkbox" ${item.isChecked ? `checked` : ``} id="checkbox" name="cardCheckbox" value=\"${item.printer_name}\" class="card-checkbox" onchange="getCurrentSelect(this)">
                        </div>
                    </div>
                    <div class="img-area">
                        <img src=\"${item.imgSrc}\" alt="" onerror="onImageError(this)">
                    </div>
                    <div class="content-area">
                        <div>
                            <span class="params-name trans" tid="t121" style="color: ${ isChangeColor ? '#7E7E7E': '#ADADAD' }">配置名称：</span>
                            <span class="params-value" title=\"${item.printer_name}\" style="color: ${ isChangeColor ? '#333': '#ADADAD' }">${item.printer_name}</span>
                        </div>
                        <div>
                            <span class="params-name trans" tid="t129" style="color: ${ isChangeColor ? '#7E7E7E': '#ADADAD' }">尺寸：</span>
                            <span class="params-value-common" style="color: ${ isChangeColor ? '#333': '#ADADAD' }">${item.dimensions.x +'*'+ item.dimensions.y +'*'+ item.dimensions.z}</span>
                        </div>
                        <div>
                            <span class="params-name trans" tid="t122" style="color: ${ isChangeColor ? '#7E7E7E': '#ADADAD' }">喷嘴直径：</span>
                            <span class="params-value-common" style="color: ${ isChangeColor ? '#333': '#ADADAD' }">${item.nozzle_diameter}</span>
                        </div>
                        <div>
                            <span class="params-name trans" tid="t123" style="color: ${ isChangeColor ? '#7E7E7E': '#ADADAD' }">固件类型：</span>
                            <span class="params-value-common" style="color: ${ isChangeColor ? '#333': '#ADADAD' }">${item.gcode_flavor}</span>
                        </div>
                    </div>
                </div>`
            $('#cardContainer').append(cardHTML)
        })
    }
    TranslatePage();
}

function onImageError (val) {
    $(val).prop('src', '../../image/printer_placeholder.png')
}

/**
 * 获取品牌下拉列表
 * @param {array} userPresetArr 
 * @returns 
 */
function getBrandSelectList (userPresetArr) {
    if (userPresetArr.length) {
        let brandList = []
        userPresetArr.forEach(item => {
            if (!brandList.some(temp => temp.vendor === item.vendor)) {
                brandList.push(item)
            }
        })
        $.each(brandList, function (index, item) {
            $('#brand').append(
                `<option value=\"${item.vendor}\">${item.vendor}</option>`
            )
        })
    }
}

/**
 * 获取机型下拉列表
 * @param {array} userPresetArr
 */
function getMachineSelectList (userPresetArr) {
    if (userPresetArr.length) {
        let machineList = []
        userPresetArr.forEach(item => {
            if (!machineList.some(temp => temp.printer_model === item.printer_model)) {
                machineList.push(item)
            }
        })
        $.each(machineList, function (index, item) {
            $('#machine').append(
                `<option value=\"${item.printer_model}\">${item.printer_model}</option>`
            )
        })
    }
}

/**
 * 监听切换品牌下拉框事件
 * @param {object} val 
 */
function changeBrand (val) {
    // 清空搜索框的值
    $('#site-search').val('')
    // 渲染用户预设列表卡片
    if (val.value === 'all') {
        $('#cardContainer').empty()
        $('#machine').empty()
        $("#machine").prepend("<option value='all'>全部</option>")
        getMachineSelectList(userPresetCurList)
        renderCard(userPresetCurList)
    } else {
        // 设置机型为all，并且重置当前品牌的全部机型
        $('#machine').val('all')
        const selectBrandList = userPresetCurList.filter(item => item.vendor === val.value)
        $('#cardContainer').empty()
        $('#machine').empty()
        $("#machine").prepend("<option value='all'>全部</option>")
        getMachineSelectList(selectBrandList)
        renderCard(selectBrandList)
    }
    isSelectAll()
}

/**
 * 切换机型
 * @param {object} val 
 */
function changeMachine (val) {
    const curBrand = $('#brand').val()
    // 清空搜索框的值
    $('#site-search').val('')
    // 1.都切换为全部
    if (curBrand === 'all' && val.value === 'all') {
        $('#cardContainer').empty()
        renderCard(userPresetCurList)
    } else if (curBrand !== 'all' && val.value !== 'all') {
    // 2.都不为全部
        const selectMachine = userPresetCurList.filter(item => {
            return (item.printer_model === val.value) && (item.vendor === curBrand)
        })
        $('#cardContainer').empty()
        renderCard(selectMachine)
    } else if (curBrand === 'all' && val.value !== 'all') {
    // 3.品牌为全部，机型不为全部
        const selectMachine = userPresetCurList.filter(item => {
            return (item.printer_model === val.value)
        })
        $('#cardContainer').empty()
        renderCard(selectMachine)
    } else if (curBrand !== 'all' && val.value === 'all' ) {
    // 4.品牌不为全部，机型为全部
        const selectMachine = userPresetCurList.filter(item => {
            return (item.vendor === curBrand)
        })
        $('#cardContainer').empty()
        renderCard(selectMachine)
    }
    isSelectAll()
}
/**
 * 搜索框
 * @param {object} val 
 */
function searchMachine (val) {
    // 防抖
    let timer = null
    if (timer) {
        clearTimeout(timer)
    }
    timer = setTimeout(() => {
        // 根据当前品牌、机型去筛选数据
        const curBrand = $('#brand').val()
        const curMachine = $('#machine').val()
        //1.当前品牌为all时、当前机型为all时：
        if (curBrand === 'all' && curMachine === 'all') {
            const searchList = userPresetCurList.filter(item => {
                if (item.printer_model.toLowerCase().includes(val.value.toLowerCase())) {
                    return item
                }
            })
            $('#cardContainer').empty()
            renderCard(searchList)
        }
        //2.当前品牌为all，机型不为all时
        else if (curBrand === 'all' && curMachine !== 'all') {
            const searchList = userPresetCurList.filter(item => {
                if (item.printer_model.toLowerCase().includes(val.value.toLowerCase()) && item.printer_model === curMachine) {
                    return item
                }
            })
            $('#cardContainer').empty()
            renderCard(searchList)
        } 
        //3.当前品牌不为all，机型为all
        else if (curBrand !== 'all' && curMachine === 'all') {
            const searchList = userPresetCurList.filter(item => {
                if (item.printer_model.toLowerCase().includes(val.value.toLowerCase()) && item.vendor === curBrand) {
                    return item
                }
            })
            $('#cardContainer').empty()
            renderCard(searchList)
        }
        //4.当前品牌不为all，机型不为al
        else {
            const searchList = userPresetCurList.filter(item => {
                if (item.printer_model.toLowerCase().includes(val.value.toLowerCase()) && item.printer_model === curMachine && item.vendor === curBrand) {
                    return item
                }
            })
            $('#cardContainer').empty()
            renderCard(searchList)
        }
    }, 100)
}
/**
 * 监听checkbox事件
 */
function selectAll () {
    const isChecked = $('#checkAll').prop('checked')
    const checkboxList = $('.card-checkbox')
    let cardList = []
    checkboxList.each(function () {
        cardList.push($(this).val())
    })
    if (isChecked) {
        // 全选当前页面打印机数据
        $('input:checkbox').each(function() { 
            $(this).prop('checked', true); 
        })
        // 当前页面所有card设置为checked状态
        userPresetCurList = userPresetCurList.map(item => {
            if (cardList.includes(item.printer_name)) {
                return {
                    ...item,
                    isChecked: true
                }
            }
            return { ...item }
        })
    } else {
        $('input:checkbox').each(function() { 
            $(this).prop('checked', false); 
        })
        // 当前页面所有card设置为false状态
        userPresetCurList = userPresetCurList.map(item => {
            if (cardList.includes(item.printer_name)) {
                return {
                    ...item,
                    isChecked: false
                }
            }
            return { ...item }
        })
    }
}
/**
 * 获取当前勾选框
 * @param {object} val 
 */
function getCurrentSelect (val) {
    const isChecked = $(val).prop('checked')
    userPresetCurList = userPresetCurList.map(item => {
        if (item.printer_name === $(val).val()) {
            return {
                ...item,
                isChecked: isChecked
            }
        }
        return item
    })
    isSelectAll()
}
/**
 * 判断是否当前checkbox全选
 */
function isSelectAll () {
    const checkboxList = $('.card-checkbox')
    let states = []
    checkboxList.each(function () {
        states.push($(this).prop('checked'))
    })
    const isCheckedAll = states.every(item => item === true)
    if (isCheckedAll) {
        $('#checkAll').prop('checked', true)
    } else {
        $('#checkAll').prop('checked', false)
    }
}


/**
 * 创建打印机
 */
function createPrinter () {
    let sendData = {
        command: "user_guide_create_printer"
    }
	SendWXMessage(JSON.stringify(sendData))
}
/**
 * 创建喷嘴
 */
function createNozzle () {
    let sendData = {
        command: "user_guide_create_nozzle"
    }
    SendWXMessage(JSON.stringify(sendData))
}